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
#include "itkMirrorPadImageFilter.h"

#include "onnxruntime_cxx_api.h"
#include "cuda_provider_factory.h"



typedef float                                 				PixelType;
const unsigned int Dimension = 3;
typedef itk::Image<PixelType, Dimension>      				ImageType;
typedef itk::ImageFileReader<ImageType>       				ReaderType;
typedef itk::ImageRegionIterator<ImageType> 			    IteratorType;

#define sizeDNNin 132
#define sizeDNNout 122

template<class T>
void executeDNN(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{

	
	ImageType::Pointer itk_img;

	typedef itk::Image<T, DIM> InputImageType;

		using FilterType = itk::CastImageFilter<InputImageType, ImageType>;
		FilterType::Pointer castFilter = FilterType::New();
		castFilter->SetInput(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
		castFilter->Update();
		itk_img = castFilter->GetOutput();
		

		ImageType::Pointer itk_img_normalized = Normamalize(itk_img);

		itk_img_normalized = AddPadding(itk_img_normalized,(sizeDNNin - sizeDNNout)/2);



	// initialize  enviroment...one enviroment per process
// enviroment maintains thread pools and other state info
	Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");

	// initialize session options if needed
	Ort::SessionOptions session_options;
	session_options.SetIntraOpNumThreads(1);

	// If onnxruntime.dll is built with CUDA enabled, we can uncomment out this line to use CUDA for this
	// session (we also need to include cuda_provider_factory.h above which defines it)
	
	OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 0);

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
	// Input 0 : dim 1 = 224
	// Input 0 : dim 2 = 224
	// Input 0 : dim 3 = 224
	// Input 0 : dim 3 = 1

	//*************************************************************************
	// Similar operations to get output node information.
	// Use OrtSessionGetOutputCount(), OrtSessionGetOutputName()
	// OrtSessionGetOutputTypeInfo() as shown above.

	//*************************************************************************
	// Score the model using sample data, and inspect values

	size_t input_tensor_size = sizeDNNin * sizeDNNin * sizeDNNin *1;  // simplify ... using known dim values to calculate size
											   // use OrtGetTensorShapeElementCount() to get official size!

	char* output_name = session.GetOutputName(0, allocator);

	std::vector<const char*> output_node_names = { output_name };

	ImageType::RegionType region = itk_img_normalized->GetLargestPossibleRegion();

	ImageType::SizeType size = region.GetSize();

	ImageType::Pointer outputImage = createImage(size[0], size[1], size[2]);

	for (int x = 0; x <= size[0] - sizeDNNin; x=x+sizeDNNout) {
		for (int y = 0; y <= size[1] - sizeDNNin; y=y+sizeDNNout) {
			for (int z = 0; z <= size[2] - sizeDNNin; z=z+sizeDNNout) {

				std::vector<float> tensor_img;

				itk2tensor(itk_img_normalized, tensor_img,x,y,z);

				// create input tensor object from data values
				auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
				Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, tensor_img.data(), input_tensor_size, input_node_dims.data(), 5);
				assert(input_tensor.IsTensor());

				// score model & input tensor, get back output tensor
				auto result = session.Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor, 1, output_node_names.data(), 1);
				assert(result.size() == 1 && result.front().IsTensor());


				//ImageType::Pointer outputImage = ImageType::New();

				tensor2itk(result, outputImage,x,y,z);
			}
		}
	}

	outputImage->SetSpacing(filter->input()[0]->itkImage()->GetSpacing());

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

}

ImageType::Pointer Normamalize(ImageType::Pointer itk_img) {

	typedef itk::NormalizeImageFilter< ImageType, ImageType > NormalizeFilterType;

	auto normalizeFilter = NormalizeFilterType::New();
	normalizeFilter->SetInput(itk_img);

	normalizeFilter->Update();

	return normalizeFilter->GetOutput();
}

ImageType::Pointer AddPadding(ImageType::Pointer itk_img, int paddingSize ) {

	ImageType::SizeType lowerExtendRegion;
	lowerExtendRegion.Fill(paddingSize);

	ImageType::SizeType upperExtendRegion;
	upperExtendRegion.Fill(paddingSize);

	using FilterType = itk::MirrorPadImageFilter<ImageType, ImageType>;
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(itk_img);
	filter->SetPadLowerBound(lowerExtendRegion);
	filter->SetPadUpperBound(upperExtendRegion);
	filter->Update();

	return filter->GetOutput();
}

bool itk2tensor(ImageType::Pointer itk_img, std::vector<float> &tensor_img, int offsetX, int offsetY, int offsetZ) {

	//ImageType::Pointer itk_img_normalized = Normamalize(itk_img);

	typename ImageType::RegionType region = itk_img->GetLargestPossibleRegion();
	

	ImageType::RegionType inputRegion;

	ImageType::RegionType::IndexType inputStart;
	ImageType::RegionType::SizeType  size;

	inputStart[0] = offsetX;
	inputStart[1] = offsetY;
	inputStart[2] = offsetZ;

	size[0] = sizeDNNin;
	size[1] = sizeDNNin;
	size[2] = sizeDNNin;

	inputRegion.SetSize(size);
	inputRegion.SetIndex(inputStart);

	tensor_img.resize(size[0] * size[1] * size[2]);
	int count = 0;
	IteratorType iter(itk_img, inputRegion);

	// convert itk to array
	for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter) {
		tensor_img[count] = iter.Get();
		count++;
	}



	return true;
}


ImageType::Pointer createImage(int X, int Y, int Z) {
	ImageType::IndexType start;
	start[0] = 0;  // first index on X
	start[1] = 0;  // first index on Y
	start[2] = 0;  // first index on Z

	ImageType::SizeType  size;
	size[0] = X;
	size[1] = Y;
	size[2] = Z;

	ImageType::RegionType region;
	region.SetSize(size);
	region.SetIndex(start);

	ImageType::Pointer itk_img = ImageType::New();
	itk_img->SetRegions(region);
	itk_img->Allocate();
	itk_img->FillBuffer(0);

	return itk_img;
}

bool tensor2itk(std::vector<Ort::Value> &tensor_img, ImageType::Pointer itk_img, int offsetX, int offsetY, int offsetZ) {



	ImageType::IndexType start;
	start[0] = offsetX;  // first index on X
	start[1] = offsetY;  // first index on Y
	start[2] = offsetZ;  // first index on Z

	ImageType::SizeType  size;
	size[0] = sizeDNNout;
	size[1] = sizeDNNout;
	size[2] = sizeDNNout;

	ImageType::RegionType region;
	region.SetSize(size);
	region.SetIndex(start);



	IteratorType iter(itk_img, region);
	int count = 0;
	// convert array to itk

	float* floatarr = tensor_img.front().GetTensorMutableData<float>();

	for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter) {
		float val = floatarr[count];
		iter.Set(val);
		count++;
	}


	return true;
}