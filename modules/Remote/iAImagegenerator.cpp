// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImagegenerator.h"

#include "iAJPGImage.h"

#include <iALog.h>

#include <vtkImageFlip.h>
#include <vtkJPEGWriter.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindowToImageFilter.h>

#include <QElapsedTimer> 

#ifdef CUDA_AVAILABLE
#include <iACudaHelper.h>
#include <nvjpeg.h>
#include <nppi_geometry_transforms.h>
#endif

// additional options:
//    - OpenCL JPEG encoder: https://github.com/roehrdor/opencl-jpeg-encoder

namespace
{

#ifdef CUDA_AVAILABLE

	void checknvj(QString const & op, nvjpegStatus_t status)
	{
		if (status != NVJPEG_STATUS_SUCCESS)
		{
			LOG(lvlError, QString("nvJPEG ERROR: operation %1: error code %2").arg(op).arg(status));
			throw std::runtime_error("JPEG generation: nvJPEG ERROR");
		}
	}

	void checkCuda(QString const & op, cudaError_t status)
	{
		if (status != cudaSuccess)
		{
			LOG(lvlError, QString("CUDA ERROR: operation %1: error code %2").arg(op).arg(status));
			throw std::runtime_error("JPEG generation: CUDA ERROR");
		}
	}

	QString nppError2String(NppStatus status)
	{
		switch (status)
		{
		case NPP_MIRROR_FLIP_ERROR: return "Invalid flip parameter";
		case NPP_SIZE_ERROR:        return "Invalid in_place ROI width or height - not even numbers!";
		case NPP_STEP_ERROR:        return "Invalid step parameter!";
		default:                    return QString("Unknown error: %1").arg(status);
		}
	}

	void checknpp(NppStatus status)
	{
		if (status != NPP_SUCCESS)
		{
			QString msg = "JPEG generation: Flip error:" + nppError2String(status);
			LOG(lvlError, msg);
			throw std::runtime_error(msg.toStdString());
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
		std::vector<unsigned char> BitmapToJpegCUDA(int width, int height, unsigned char* buffer, int quality, QString & debugMsg)
		{
			checknvj("encoderParamsSetSamplingFactors", nvjpegEncoderParamsSetSamplingFactors(nv_enc_params, NVJPEG_CSS_444, stream));
			checknvj("encoderParamsSetQuality", nvjpegEncoderParamsSetQuality(nv_enc_params, quality, stream));

			unsigned char* pCudaBuffer = nullptr;
			checkCuda("malloc", cudaMalloc(reinterpret_cast<void**>(& pCudaBuffer), width * height * NVJPEG_MAX_COMPONENT));

			auto inputImageBytes = width * height * 3;
		
			checkCuda("memCpy", cudaMemcpy(pCudaBuffer, buffer, inputImageBytes, cudaMemcpyHostToDevice));

			if (width % 2 == 0 && height % 2 == 0)   // for npp mirror operation, both width and height need to be even
			{
				QElapsedTimer flipTimer; flipTimer.start();
				int lineStep = width * 3;
				NppiSize oROI;
				oROI.height = height;
				oROI.width = width;
				NppiAxis flip = NPP_HORIZONTAL_AXIS;  // a bit confusing - horizontal axis means switch vertically, i.e. flip y
				// NppStatus nppiMirror_8u_C3IR(Npp8u *pSrcDst, int nSrcDstStep, NppiSize oROI, NppiAxis flip)
				checknpp(nppiMirror_8u_C3IR(pCudaBuffer, lineStep, oROI, flip));
				debugMsg += QString("; nppiMirror: %1 ms").arg(flipTimer.elapsed());
			}

			// for "planar" memory layout:
			//for (int i = 0; i < NVJPEG_MAX_COMPONENT; ++i)
			//{
			//	nv_image.channel[i] = pCudaBuffer + width * height * i;
			//	nv_image.pitch[i] = (unsigned int)width;
			//}

			// interleaved:
			nvjpegImage_t nv_image;
			// for "interleaved" memory layout:
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

	std::shared_ptr<iAJPGImage> nvJPEGCreateImage(QString viewID, vtkRenderWindow* window, int quality)
	{
		QElapsedTimer t1; t1.start();
		static iACudaImageGen cudaImageGen;
		vtkNew<vtkWindowToImageFilter> w2if;    // grabs RGB image
		w2if->ShouldRerenderOff();
		w2if->SetInput(window);
		w2if->Update();
		auto renderTime = window->GetRenderers()->GetFirstRenderer()->GetLastRenderTimeInSeconds() * 1000;

		// nvidia expects image flipped around y axis in comparison to VTK!
		auto vtkImg = w2if->GetOutput();
		auto const dim = vtkImg->GetDimensions();
		QString debugMsg = QString("CUDA JPEG from %1 view (%2x%3); grab: %4 ms; last render: %5 ms")
			.arg(viewID).arg(dim[0]).arg(dim[1]).arg(t1.elapsed()).arg(renderTime);
		vtkNew<vtkImageFlip> flipYFilter;    // outside of if to avoid memory problems (release of output when filter goes out of scope)
		if (dim[0] % 2 != 0 || dim[1] % 2 != 0)    // for CUDA-accelerated flip, both sizes need to be divisible by 2 (see also BitmapToJpegCUDA)
		{
			QElapsedTimer tFlip; tFlip.start();
			auto image = w2if->GetOutput();
			flipYFilter->SetFilteredAxis(1); // flip y axis
			flipYFilter->SetInputData(image);
			flipYFilter->Update();
			vtkImg = flipYFilter->GetOutput();
			debugMsg += QString("; VTK flip: %1 ms").arg(tFlip.elapsed());
		}

		QElapsedTimer t2; t2.start();
		unsigned char* buffer = static_cast<unsigned char*>(vtkImg->GetScalarPointer());
		auto extractTime = t2.elapsed();
		assert(dim[2] == 1);

		QElapsedTimer t3; t3.start();
		auto data = cudaImageGen.BitmapToJpegCUDA(dim[0], dim[1], buffer, quality, debugMsg);
		LOG(lvlDebug, QString("%1; extract: %2 ms; nvJPEG: %3 ms; size: %4 kB")
			.arg(debugMsg).arg(extractTime).arg(t3.elapsed()).arg(data.size()/1000.0));
		auto result = std::make_shared<iAJPGImage>();
		result->data = QByteArray(reinterpret_cast<char*>(data.data()), data.size());
		result->width = dim[0];
		result->height = dim[1];
		return result;
	}

#endif

	std::shared_ptr<iAJPGImage> vtkTurboJPEGCreateImage(vtkRenderWindow* window, int quality)
	{
		QElapsedTimer t1; t1.start();
		vtkNew<vtkWindowToImageFilter> w2if;
		w2if->ShouldRerenderOff();
		w2if->SetInput(window);
		w2if->Update();
		auto renderTime = window->GetRenderers()->GetFirstRenderer()->GetLastRenderTimeInSeconds() * 1000;
		auto grabTime = t1.elapsed();
		auto img = w2if->GetOutput();

		QElapsedTimer t; t.start();
		vtkNew<vtkJPEGWriter> writer;
		writer->SetInputConnection(w2if->GetOutputPort());
		writer->SetQuality(quality);
		writer->WriteToMemoryOn();
		writer->Write();
		vtkSmartPointer<vtkUnsignedCharArray> imgData = writer->GetResult();
		auto result = std::make_shared<iAJPGImage>();
		result->data = QByteArray((char*)imgData->Begin(), static_cast<qsizetype>(imgData->GetSize()));
		result->width = img->GetDimensions()[0];
		result->height = img->GetDimensions()[1];
		LOG(lvlDebug, QString("VTK JPEG. grab: %1 ms; last render: %2 ms; jpeg: %3 ms; size: %4 kB")
			.arg(grabTime).arg(renderTime).arg(t.elapsed()).arg(imgData->GetSize()/1000.0));
		return result;
	}
}

namespace iAImagegenerator
{
	std::shared_ptr<iAJPGImage> createImage(QString const & viewID, vtkRenderWindow* window, int quality)
	{
#if CUDA_AVAILABLE
		static bool cudaWorking = isCUDAAvailable();
		if (cudaWorking)
		{
			try
			{
				return nvJPEGCreateImage(viewID, window, quality);
			}
			catch (std::runtime_error& /*e*/)  // in case of problems with CUDA / nvJPEG (e.g. unsupported driver, missing libraries, ...)
			{
				LOG(lvlWarn, QString("There were problems with running nvJPEG (see above). Falling back to vtkTurboJPEG for the moment. "
					"If possible, fix the above errors and restart open_iA to try again to use faster nvJPEG library!"));
				cudaWorking = false;           // if it fails once, disable future use of nvJPEG for this run of open_iA
			}
		}
#endif
		return vtkTurboJPEGCreateImage(window, quality);
	}
}
