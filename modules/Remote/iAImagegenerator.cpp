/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAImagegenerator.h"

#include <iALog.h>

#include <vtkJPEGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindowToImageFilter.h>

#ifdef USE_CUDA
#include <vtkImageFlip.h>

#include <nvjpeg.h>

#include <QElapsedTimer>

namespace
{
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


QByteArray iAImagegenerator::createImage(vtkRenderWindow* window, int quality)
{
	QElapsedTimer t1; t1.start();
	static iACudaImageGen cudaImageGen;
	vtkNew<vtkWindowToImageFilter> w2if;
	w2if->ShouldRerenderOff();
	w2if->SetInput(window);
	w2if->Update();
	LOG(lvlDebug, QString("grab: %1 ms").arg(t1.elapsed()));

	QElapsedTimer t2; t2.start();
	// nvidia expects image flipped around y axis in comparison to VTK!
	vtkNew<vtkImageFlip> flipYFilter;
	flipYFilter->SetFilteredAxis(1); // flip y axis
	flipYFilter->SetInputConnection(w2if->GetOutputPort());
	flipYFilter->Update();

	auto vtkImg = flipYFilter->GetOutput();
	auto const dim = vtkImg->GetDimensions();
	unsigned char * buffer = static_cast<unsigned char*>(vtkImg->GetScalarPointer());
	assert(dim[2] == 1);
	LOG(lvlDebug, QString("grab: %1 ms").arg(t2.elapsed()));

	QElapsedTimer t3; t3.start();
	auto data = cudaImageGen.BitmapToJpegCUDA(dim[0], dim[1], buffer, quality);
	LOG(lvlDebug, QString("nvJPEG: %1 ms").arg(t3.elapsed()));
	return QByteArray(reinterpret_cast<char*>(data.data()), data.size());
}

#else

QByteArray iAImagegenerator::createImage(vtkRenderWindow* window, int quality)
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
	vtkSmartPointer<vtkUnsignedCharArray> img = writer->GetResult();
	QByteArray result((char*)data->Begin(), static_cast<qsizetype>(data->GetSize()));
	LOG(lvlDebug, QString("turboJPEG: %1 ms").arg(t.elapsed()));
	return result;
}

#endif
