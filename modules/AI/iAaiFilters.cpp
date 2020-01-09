/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAaiFilters.h"

#include "defines.h" // for DIM

#include "iAConnector.h"
#include "iAConsole.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include "itkCastImageFilter.h"
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkNormalizeImageFilter.h"
#include "itkImageRegionIterator.h"

#include "onnxruntime_cxx_api.h"



typedef float                                 				PixelType;
const unsigned int Dimension = 3;
typedef itk::Image<PixelType, Dimension>      				ImageType;
typedef itk::ImageFileReader<ImageType>       				ReaderType;
typedef itk::ImageRegionIterator<ImageType> 			    IteratorType;


template<class T>
void executeDNN(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{

	std::vector<float> tensor_img;

	typedef itk::Image<T, DIM> InputImageType;
	if (filter->input()[0]->itkScalarPixelType() != itk::ImageIOBase::FLOAT) {
		using FilterType = itk::CastImageFilter<InputImageType, ImageType>;
		FilterType::Pointer castFilter = FilterType::New();
		castFilter->SetInput(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
		castFilter->Update();
		itk2tensor(dynamic_cast<ImageType *>(castFilter->GetOutput()), tensor_img);
		
	}
	else {
		itk2tensor(dynamic_cast<ImageType *>(filter->input()[0]->itkImage()), tensor_img);
	}

	// initialize  enviroment...one enviroment per process
// enviroment maintains thread pools and other state info
	Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");

	// initialize session options if needed
	Ort::SessionOptions session_options;
	session_options.SetIntraOpNumThreads(1);

	// If onnxruntime.dll is built with CUDA enabled, we can uncomment out this line to use CUDA for this
	// session (we also need to include cuda_provider_factory.h above which defines it)
	// #include "cuda_provider_factory.h"
	// OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 1);

	// Sets graph optimization level
	// Available levels are
	// ORT_DISABLE_ALL -> To disable all optimizations
	// ORT_ENABLE_BASIC -> To enable basic optimizations (Such as redundant node removals)
	// ORT_ENABLE_EXTENDED -> To enable extended optimizations (Includes level 1 + more complex optimizations like node fusions)
	// ORT_ENABLE_ALL -> To Enable All possible opitmizations
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

	//*************************************************************************
	// create session and load model into memory
	// using squeezenet version 1.3
	// URL = https://github.com/onnx/models/tree/master/squeezenet
//#ifdef _WIN32
//	 wchar_t* model_path = L"C:\\Users\\p41877\\Downloads\\squeezenet\\model.onnx";
//#else
//	const char* model_path = "squeezenet.onnx";
//#endif
//
//

	wchar_t model_path[128];

	parameters["OnnxFile"].toString().toWCharArray(model_path);
	model_path[parameters["OnnxFile"].toString().length()] = L'\0';
	DEBUG_LOG(QString("Using Onnxruntime C++ API"));
	Ort::Session session(env, model_path, session_options);

	//*************************************************************************
	// print model input layer (node names, types, shape etc.)
	Ort::AllocatorWithDefaultOptions allocator;

	// print number of model input nodes
	size_t num_input_nodes = session.GetInputCount();
	std::vector<const char*> input_node_names(num_input_nodes);
	std::vector<int64_t> input_node_dims;  // simplify... this model has only 1 input node {1, 3, 224, 224}.
										   // Otherwise need vector<vector<>>

	DEBUG_LOG(QString("Number of inputs = %1").arg(num_input_nodes));

	// iterate over all input nodes
	for (int i = 0; i < num_input_nodes; i++) {
		// print input node names
		char* input_name = session.GetInputName(i, allocator);
		DEBUG_LOG(QString("Input %1 : name=%2").arg(i).arg(input_name));
		input_node_names[i] = input_name;

		// print input node types
		Ort::TypeInfo type_info = session.GetInputTypeInfo(i);
		auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

		ONNXTensorElementDataType type = tensor_info.GetElementType();
		DEBUG_LOG(QString("Input %1 : type=%2").arg(i).arg(type));

		// print input shapes/dims
		input_node_dims = tensor_info.GetShape();
		DEBUG_LOG(QString("Input %1 : num_dims=%2").arg(i).arg(input_node_dims.size()));
		for (int j = 0; j < input_node_dims.size(); j++) {
			DEBUG_LOG(QString("Input %1 : dim %2=%3").arg(i).arg(j).arg(input_node_dims[j]));
			if (input_node_dims[j] == -1) {
				input_node_dims[j] = 1;
			}
		}


	}

	// Results should be...
	// Number of inputs = 1
	// Input 0 : name = data_0
	// Input 0 : type = 1
	// Input 0 : num_dims = 4
	// Input 0 : dim 0 = 1
	// Input 0 : dim 1 = 3
	// Input 0 : dim 2 = 224
	// Input 0 : dim 3 = 224

	//*************************************************************************
	// Similar operations to get output node information.
	// Use OrtSessionGetOutputCount(), OrtSessionGetOutputName()
	// OrtSessionGetOutputTypeInfo() as shown above.

	//*************************************************************************
	// Score the model using sample data, and inspect values

	size_t input_tensor_size = 128 * 128 * 128*1;  // simplify ... using known dim values to calculate size
											   // use OrtGetTensorShapeElementCount() to get official size!

	std::vector<const char*> output_node_names = { "conv3d_24" };



	// create input tensor object from data values
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, tensor_img.data(), input_tensor_size, input_node_dims.data(), 5);
	assert(input_tensor.IsTensor());

	// score model & input tensor, get back output tensor
	auto result = session.Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor, 1, output_node_names.data(), 1);
	assert(result.size() == 1 && result.front().IsTensor());


	ImageType::Pointer outputImage = ImageType::New();
	
	tensor2itk(result, outputImage, 128, 128, 128);

	filter->addOutput(outputImage);


}

IAFILTER_CREATE(iAai)

void iAai::performWork(QMap<QString, QVariant> const & parameters) {
	ITK_TYPED_CALL(executeDNN, input()[0]->itkScalarPixelType(), this, parameters);
}

iAai::iAai() :
	iAFilter("AI", "Segmentation",
		"Uses deep learning model for segmentation<br/>"
		"ONNX Runtime is used for execution of the net"
		"<a href=\"https://github.com/microsoft/onnxruntime\">")
{

	addParameter("OnnxFile", FileNameOpen);
	addParameter("STL output filename", String, "");

}

ImageType::Pointer Normamalize(ImageType::Pointer itk_img) {

	typedef itk::NormalizeImageFilter< ImageType, ImageType > NormalizeFilterType;

	auto normalizeFilter = NormalizeFilterType::New();
	normalizeFilter->SetInput(itk_img);

	normalizeFilter->Update();

	return normalizeFilter->GetOutput();
}

bool itk2tensor(ImageType::Pointer itk_img, std::vector<float> &tensor_img) {

	itk_img = Normamalize(itk_img);

	typename ImageType::RegionType region = itk_img->GetLargestPossibleRegion();
	const typename ImageType::SizeType size = region.GetSize();


	tensor_img.resize(size[0] * size[1] * size[2]);
	int count = 0;
	IteratorType iter(itk_img, itk_img->GetRequestedRegion());

	// convert itk to array
	for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter) {
		tensor_img[count] = iter.Get();
		count++;
	}



	return true;
}


bool tensor2itk(std::vector<Ort::Value> &tensor_img, ImageType::Pointer itk_img, int x, int y, int z) {



	ImageType::IndexType start;
	start[0] = 0;  // first index on X
	start[1] = 0;  // first index on Y
	start[2] = 0;  // first index on Z

	ImageType::SizeType  size;
	size[0] = x;
	size[1] = y;
	size[2] = z;

	ImageType::RegionType region;
	region.SetSize(size);
	region.SetIndex(start);

	itk_img->SetRegions(region);
	itk_img->Allocate();

	int len = size[0] * size[1] * size[2];

	IteratorType iter(itk_img, itk_img->GetRequestedRegion());
	int count = 0;
	// convert array to itk

	float* floatarr = tensor_img.front().GetTensorMutableData<float>();

	for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter) {
		iter.Set(floatarr[count]);
		count++;
	}


	return true;
}