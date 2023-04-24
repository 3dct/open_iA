// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImagegenerator.h"

#include <iALog.h>

#include <vtkJPEGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindowToImageFilter.h>

#include <QElapsedTimer> 

#ifdef CUDA_AVAILABLE
#include <iACudaHelper.h>
#include <vtkImageFlip.h>
#include <nvjpeg.h>
#endif

namespace
{

#ifdef CUDA_AVAILABLE

	void checknvj(QString const & op, nvjpegStatus_t status)
	{
		if (status != NVJPEG_STATUS_SUCCESS)
		{
			LOG(lvlError, QString("nvJPEG ERROR: operation %1: error code %2").arg(op).arg(status));
		}
	}

	void checkCuda(QString const & op, cudaError_t status)
	{
		if (status != cudaSuccess)
		{
			LOG(lvlError, QString("CUDA ERROR: operation %1: error code %2").arg(op).arg(status));
		}
	}

	class iACudaImageGen
	{
	public:
		iACudaImageGen()
		{
			// seems we don't need to create a stream, at least the resize example passes NULL as stream and works...
			//checkCuda("streamCreate", cudaStreamCreate(&stream));
			checknvj("createSimple", nvjpegCreateSimple(&nv_handle));
			checknvj("encoderStateCreate", nvjpegEncoderStateCreate(nv_handle, &nv_enc_state, stream));
			checknvj("encoderParamsCreate", nvjpegEncoderParamsCreate(nv_handle, &nv_enc_params, stream));

		}
		~iACudaImageGen()
		{
			// for some reason, nvjpegEncoderParamsDestroy causes crash?
			//checknvj("encoderParamsDestroy", nvjpegEncoderParamsDestroy(nv_enc_params));
			//checknvj("encoderStateDestroy", nvjpegEncoderStateDestroy(nv_enc_state));
			//checknvj("Destroy", nvjpegDestroy(nv_handle));

			//checkCuda("streamDestroy", cudaStreamDestroy(stream));
		}
		std::vector<unsigned char> BitmapToJpegCUDA(int width, int height, unsigned char* buffer, int quality)
		{
			checknvj("encoderParamsSetSamplingFactors", nvjpegEncoderParamsSetSamplingFactors(nv_enc_params, NVJPEG_CSS_444, stream));
			checknvj("encoderParamsSetQuality", nvjpegEncoderParamsSetQuality(nv_enc_params, quality, stream));

			// interleaved:
			nvjpegImage_t nv_image;

			unsigned char* pCudaBuffer = nullptr;
			checkCuda("malloc", cudaMalloc(reinterpret_cast<void**>(& pCudaBuffer), width * height * NVJPEG_MAX_COMPONENT));

			auto inputImageBytes = width * height * 3;
		
			checkCuda("memCpy", cudaMemcpy(pCudaBuffer, buffer, inputImageBytes, cudaMemcpyHostToDevice));

			// for "planar" memory layout:
			//for (int i = 0; i < NVJPEG_MAX_COMPONENT; ++i)
			//{
			//	nv_image.channel[i] = pCudaBuffer + width * height * i;
			//	nv_image.pitch[i] = (unsigned int)width;
			//}

			// for "interleaved" memory layou:
			nv_image.channel[0] = pCudaBuffer;
			nv_image.pitch[0] = width * 3;

			checknvj("encodeImage", nvjpegEncodeImage(nv_handle, nv_enc_state, nv_enc_params, &nv_image,
				NVJPEG_INPUT_RGBI, width, height, stream));

			size_t length = 0;
			checknvj("retrieveBitstream(nullptr)", nvjpegEncodeRetrieveBitstream(nv_handle, nv_enc_state, nullptr, &length, stream));

			checkCuda("streamSync", cudaStreamSynchronize(stream));
			std::vector<unsigned char> jpeg(length);
			checknvj("retrieveBitstream", nvjpegEncodeRetrieveBitstream(nv_handle, nv_enc_state, jpeg.data(), &length, 0));

			return jpeg;
		}
	public:
		nvjpegHandle_t nv_handle;
		nvjpegEncoderState_t nv_enc_state;
		nvjpegEncoderParams_t nv_enc_params;
		cudaStream_t stream = nullptr;
	};

	QByteArray nvJPEGCreateImage(vtkRenderWindow* window, int quality)
	{
		QElapsedTimer t1; t1.start();
		static iACudaImageGen cudaImageGen;
		vtkNew<vtkWindowToImageFilter> w2if;
		w2if->ShouldRerenderOff();
		w2if->SetInput(window);
		w2if->Update();

		QElapsedTimer t2; t2.start();
		// nvidia expects image flipped around y axis in comparison to VTK!

		auto image = w2if->GetOutput();
		vtkNew<vtkImageFlip> flipYFilter;
		flipYFilter->SetFilteredAxis(1); // flip y axis
		flipYFilter->SetInputData(image);
		flipYFilter->Update();

		auto vtkImg = flipYFilter->GetOutput();
		auto const dim = vtkImg->GetDimensions();
		unsigned char* buffer = static_cast<unsigned char*>(vtkImg->GetScalarPointer());
		assert(dim[2] == 1);
		//LOG(lvlDebug, QString("grab: %1 ms").arg(t2.elapsed()));

		QElapsedTimer t3; t3.start();
		auto data = cudaImageGen.BitmapToJpegCUDA(dim[0], dim[1], buffer, quality);
		//LOG(lvlDebug, QString("nvJPEG: %1 ms").arg(t3.elapsed()));
		return QByteArray(reinterpret_cast<char*>(data.data()), data.size());
	}

#endif

	QByteArray vtkTurboJPEGCreateImage(vtkRenderWindow* window, int quality)
	{
		vtkNew<vtkWindowToImageFilter> w2if;
		w2if->ShouldRerenderOff();
		w2if->SetInput(window);
		w2if->Update();

		QElapsedTimer t; t.start();
		vtkNew<vtkJPEGWriter> writer;
		writer->SetInputConnection(w2if->GetOutputPort());
		writer->SetQuality(quality);
		writer->WriteToMemoryOn();
		writer->Write();
		vtkSmartPointer<vtkUnsignedCharArray> imgData = writer->GetResult();
		QByteArray result((char*)imgData->Begin(), static_cast<qsizetype>(imgData->GetSize()));
		//LOG(lvlDebug, QString("turboJPEG: %1 ms").arg(t.elapsed()));
		return result;
	}
}


QByteArray iAImagegenerator::createImage(vtkRenderWindow* window, int quality)
{
#if CUDA_AVAILABLE
	if (isCUDAAvailable())
	{
		return nvJPEGCreateImage(window, quality);
	}
#endif
	return vtkTurboJPEGCreateImage(window, quality);
}
