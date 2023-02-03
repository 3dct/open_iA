// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h> // for DIM
#include <iADataSet.h>
#include <iAFilterDefault.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iAStringHelper.h>   // for joinStdString
#include <iATypedCallHelper.h>

#include <itkCastImageFilter.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkNormalizeImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkMirrorPadImageFilter.h>

#include <onnxruntime_cxx_api.h>

#ifndef ONNX_CUDA
	#include <dml_provider_factory.h>
#endif

#include <omp.h>

#include <vector>


IAFILTER_DEFAULT_CLASS(iAai);

typedef float                                 				PixelType;
const unsigned int Dimension = 3;
typedef itk::Image<PixelType, Dimension>      				ImageType;
typedef itk::ImageFileReader<ImageType>       				ReaderType;
typedef itk::ImageRegionIterator<ImageType> 			    IteratorType;

namespace
{
	//const int sizeDNNin = 132;
	//const int sizeDNNout = 122;
}

typename ImageType::Pointer Normalize(typename ImageType::Pointer itk_img)
{
	typedef itk::NormalizeImageFilter< ImageType, ImageType > NormalizeFilterType;

	auto normalizeFilter = NormalizeFilterType::New();
	normalizeFilter->SetInput(itk_img);

	normalizeFilter->Update();

	return normalizeFilter->GetOutput();
}

typename ImageType::Pointer AddPadding(typename ImageType::Pointer itk_img, int paddingSize )
{
	ImageType::SizeType lowerExtendRegion;
	lowerExtendRegion.Fill(paddingSize);

	ImageType::SizeType upperExtendRegion;
	upperExtendRegion.Fill(paddingSize);

	using FilterType = itk::MirrorPadImageFilter<ImageType, ImageType>;
	typename FilterType::Pointer filter = FilterType::New();
	filter->SetInput(itk_img);
	filter->SetPadLowerBound(lowerExtendRegion);
	filter->SetPadUpperBound(upperExtendRegion);
	filter->Update();

	return filter->GetOutput();
}

bool itk2tensor(typename ImageType::Pointer itk_img, std::vector<float> &tensor_img, int offsetX, int offsetY, int offsetZ, int sizeDNNin)
{
	//typename ImageType::Pointer itk_img_normalized = Normalize(itk_img);

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
	for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter)
	{
		tensor_img[count] = iter.Get();
		count++;
	}
	return true;
}

bool tensor2itk(std::vector<Ort::Value> &tensor_img, typename ImageType::Pointer itk_img, int offsetX, int offsetY, int offsetZ, int sizeDNNout, int offset = 0, int chanels=1)
{
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

	for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter)
	{
		
		count = count+offset;
		float val = floatarr[count];
		iter.Set(val);
		count = count + ( chanels - offset);
	}

	return true;
}

typename ImageType::Pointer createImage(int X, int Y, int Z)
{
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

	typename ImageType::Pointer itk_img = ImageType::New();
	itk_img->SetRegions(region);
	itk_img->Allocate();
	itk_img->FillBuffer(0);

	return itk_img;
}

template<class T>
void executeDNN(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<T, DIM> InputImageType;

	using FilterType = itk::CastImageFilter<InputImageType, ImageType>;
	typename FilterType::Pointer castFilter = FilterType::New();
	castFilter->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	castFilter->Update();
	auto itk_img = castFilter->GetOutput();



	// initialize  enviroment...one enviroment per process
	// enviroment maintains thread pools and other state info
	Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Default");

	// initialize session options if needed
	Ort::SessionOptions session_options;

	// If onnxruntime.dll is built with CUDA enabled, we can uncomment out this line to use CUDA for this
	// session (we also need to include cuda_provider_factory.h above which defines it)
	
	if (parameters["use GPU"].toBool())
	{
#ifdef ONNX_CUDA
		OrtCUDAProviderOptions options;
		session_options.AppendExecutionProvider_CUDA(options);
#else
		//session_options.DisableMemPattern();
		//session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
		Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(session_options, parameters["GPU"].toInt()));
#endif
	}
	// Sets graph optimization level
	// Available levels are
	// ORT_DISABLE_ALL -> To disable all optimizations
	// ORT_ENABLE_BASIC -> To enable basic optimizations (Such as redundant node removals)
	// ORT_ENABLE_EXTENDED -> To enable extended optimizations (Includes level 1 + more complex optimizations like node fusions)
	// ORT_ENABLE_ALL -> To Enable All possible opitmizations
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

	//*************************************************************************
	// create session and load model into memory

	#ifdef _WIN32
		wchar_t model_path[128];
		parameters["OnnxFile"].toString().toWCharArray(model_path);
		model_path[parameters["OnnxFile"].toString().length()] = L'\0';
	#else
		std::string modelPathStdString = parameters["OnnxFile"].toString().toStdString();
		const char* model_path = modelPathStdString.c_str();
	#endif

	LOG(lvlInfo, QString("Using Onnxruntime C++ API"));
	Ort::Session session(env, model_path, session_options);

	//*************************************************************************
	// print model input layer (node names, types, shape etc.)
	Ort::AllocatorWithDefaultOptions allocator;

	// print number of model input nodes
	size_t num_input_nodes = session.GetInputCount();
#ifdef ONNX_NEWNAMEFUNCTIONS
	std::vector<Ort::AllocatedStringPtr> input_node_names_smartptrs;
#endif
	std::vector<const char*> input_node_names(num_input_nodes);
	std::vector<int64_t> input_node_dims;  // simplify... this model has only 1 input node {1, 3, 224, 224}.
										   // Otherwise need vector<vector<>>

	LOG(lvlInfo, QString("Number of inputs = %1").arg(num_input_nodes));

	// iterate over all input nodes
	for (size_t i = 0; i < num_input_nodes; i++)
	{
		// print input node names
#ifdef ONNX_NEWNAMEFUNCTIONS
		input_node_names_smartptrs.emplace_back(session.GetInputNameAllocated(i, allocator));
		auto input_name = input_node_names_smartptrs[i].get();
#else
		char* input_name = session.GetInputName(i, allocator);
#endif
		LOG(lvlInfo, QString("Input %1 : name=%2").arg(i).arg(input_name));
		input_node_names[i] = input_name;

		// print input node types
		Ort::TypeInfo type_info = session.GetInputTypeInfo(i);
		auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

		ONNXTensorElementDataType type = tensor_info.GetElementType();
		LOG(lvlInfo, QString("Input %1 : type=%2").arg(i).arg(type));

		// print input shapes/dims
		input_node_dims = tensor_info.GetShape();
		LOG(lvlInfo, QString("Input %1 : num_dims=%2").arg(i).arg(input_node_dims.size()));
		for (size_t j = 0; j < input_node_dims.size(); j++)
		{
			LOG(lvlInfo, QString("Input %1 : dim %2=%3").arg(i).arg(j).arg(input_node_dims[j]));
			if (input_node_dims[j] == -1)
			{
				input_node_dims[j] = 1;
			}
		}
	}

	// print number of model input nodes
	size_t num_output_nodes = session.GetOutputCount();
#if ONNX_NEWNAMEFUNCTIONS
	std::vector<Ort::AllocatedStringPtr> output_node_names_smartptrs;
#endif
	std::vector<const char*> output_node_names(num_output_nodes);
	std::vector<int64_t> output_node_dims;  // simplify... this model has only 1 input node {1, 3, 224, 224}.
										   // Otherwise need vector<vector<>>

	LOG(lvlInfo, QString("Number of outputs = %1").arg(num_output_nodes));

	// iterate over all input nodes
	for (size_t i = 0; i < num_output_nodes; i++)
	{
		// print input node names
#if ONNX_NEWNAMEFUNCTIONS
		output_node_names_smartptrs.emplace_back(session.GetOutputNameAllocated(i, allocator));
		auto output_name = output_node_names_smartptrs[i].get();
#else
		char* output_name = session.GetOutputName(i, allocator);
#endif
		LOG(lvlInfo, QString("Output %1 : name=%2").arg(i).arg(output_name));
		output_node_names[i] = output_name;

		// print input node types
		Ort::TypeInfo type_info = session.GetOutputTypeInfo(i);
		auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

		ONNXTensorElementDataType type = tensor_info.GetElementType();
		LOG(lvlInfo, QString("Output %1 : type=%2").arg(i).arg(type));

		// print input shapes/dims
		output_node_dims = tensor_info.GetShape();
		LOG(lvlInfo, QString("Output %1 : num_dims=%2").arg(i).arg(output_node_dims.size()));
		for (size_t j = 0; j < output_node_dims.size(); j++)
		{
			LOG(lvlInfo, QString("Output %1 : dim %2=%3").arg(i).arg(j).arg(output_node_dims[j]));
			if (output_node_dims[j] == -1)
			{
				output_node_dims[j] = 1;
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

	int sizeDNNin = input_node_dims[1];
	int sizeDNNout = output_node_dims[1];

	typename ImageType::Pointer itk_img_normalized = Normalize(itk_img);
	typename ImageType::Pointer itk_img_normalized_padded =
		AddPadding(itk_img_normalized, (sizeDNNin - sizeDNNout) / 2);

	//*************************************************************************
	// Similar operations to get output node information.
	// Use OrtSessionGetOutputCount(), OrtSessionGetOutputName()
	// OrtSessionGetOutputTypeInfo() as shown above.

	//*************************************************************************
	// Score the model using sample data, and inspect values

	size_t input_tensor_size = sizeDNNin * sizeDNNin * sizeDNNin *1;  // simplify ... using known dim values to calculate size
											   // use OrtGetTensorShapeElementCount() to get official size!

	ImageType::RegionType region = itk_img_normalized->GetLargestPossibleRegion();

	ImageType::SizeType size = region.GetSize();

	std::list<ImageType::Pointer> outputs; 

	Ort::TypeInfo type_info = session.GetOutputTypeInfo(0);
	auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
	for (int i = 0; i < tensor_info.GetShape()[output_node_dims.size()-1]; i++)
	{
		outputs.push_back(createImage(size[0], size[1], size[2]));
	}

	int sizeX = size[0];
	int sizeY = size[1];
	int sizeZ = size[2];

	iAProgress *progressPrediction = filter->progress();
	int count = 0;

	for (int x = 0; x <= sizeX; x = x + sizeDNNout)
	{
		for (int y = 0; y <= sizeY; y = y + sizeDNNout)
		{
			std::vector<std::string> errors;
#pragma omp parallel for
			for (int z = 0; z <= sizeZ; z = z + sizeDNNout)
			{
				try  // exceptions must not exit the openmp block! see, e.g., https://pvs-studio.com/en/blog/posts/0008/
				{
					std::vector<float> tensor_img;

					int tempX, tempY, tempZ;

					tempX = (x <= sizeX - sizeDNNout) ? x : (x - (sizeDNNout - sizeX % sizeDNNout));
					tempY = (y <= sizeY - sizeDNNout) ? y : (y - (sizeDNNout - sizeY % sizeDNNout));
					tempZ = (z <= sizeZ - sizeDNNout) ? z : (z - (sizeDNNout - sizeZ % sizeDNNout));

					int offset = (sizeDNNout - sizeDNNin) / 2;
					itk2tensor(itk_img_normalized_padded, tensor_img, tempX + offset, tempY + offset, tempZ + offset, sizeDNNin);

					std::vector<Ort::Value> result;

					// create input tensor object from data values
					auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
					Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
						memory_info, tensor_img.data(), input_tensor_size, input_node_dims.data(), 5);
					assert(input_tensor.IsTensor());
					bool running = true;
#pragma omp critical
					{
						try  // exceptions must not exit the openmp block! see, e.g., https://stackoverflow.com/questions/13663231
						{
							// score model & input tensor, get back output tensor
							result = session.Run(Ort::RunOptions{nullptr}, input_node_names.data(), &input_tensor, 1,
								output_node_names.data(), 1);
						}
						catch (const std::exception& e)
						{
							errors.push_back(e.what());
							running = false;
						}
					}
					if (running)
					{
						assert(result.size() == 1 && result.front().IsTensor());

						int outputChannel = 0;
						//ImageType::Pointer outputImage = ImageType::New();
						for (auto outputImage : outputs)
						{
							tensor2itk(result, outputImage, tempX, tempY, tempZ, sizeDNNout,outputChannel,outputs.size());
							outputChannel++;
						}
						count++;
						double progress = count * 100.0 / (sizeX / sizeDNNout * sizeY / sizeDNNout * sizeZ / sizeDNNout);
						progressPrediction->emitProgress(progress);
					}
				}
				catch (const std::exception& e)
				{
					errors.push_back(e.what());
				}
			}
			if (errors.size() > 0)
			{
				throw std::runtime_error(joinStdString(errors, ", "));
			}
		}
	}
	for (auto outputImage : outputs)
	{
		outputImage->SetSpacing(filter->imageInput(0)->itkImage()->GetSpacing());
		filter->addOutput(outputImage);
	}
}

void iAai::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(executeDNN, inputScalarType(), this, parameters);
}

iAai::iAai() :
	iAFilter("AI", "Segmentation",
		"Uses deep learning model for segmentation<br/>"
		"ONNX Runtime is used for execution of the net"
		"<a href=\"https://github.com/microsoft/onnxruntime\">"
		"GPU select gpu should be used by DirectML (0 -> Default GPU)")
{

	addParameter("OnnxFile", iAValueType::FileNameOpen);
	addParameter("use GPU", iAValueType::Boolean, true);
	addParameter("GPU", iAValueType::Discrete,0);
	
}
