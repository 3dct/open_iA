// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h> // for DIM
#include <iADataSet.h>
#include <iAFileUtils.h>
#include <iAFilterDefault.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkConnectedComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>

IAFILTER_DEFAULT_CLASS(iAConnectedComponents);
IAFILTER_DEFAULT_CLASS(iAScalarConnectedComponents);
IAFILTER_DEFAULT_CLASS(iARelabelComponents);

template<class T>
void connectedComponentFilter(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<long, DIM> OutputImageType;
	typedef itk::ConnectedComponentImageFilter< InputImageType, OutputImageType > CCIFType;
	auto ccFilter = CCIFType::New();
	ccFilter->SetInput( dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()) );
	ccFilter->SetBackgroundValue(0);
	ccFilter->SetFullyConnected(parameters["Fully Connected"].toBool());
	filter->progress()->observe(ccFilter);
	ccFilter->Update();
	auto out = ccFilter->GetOutput();
	// by default, at least ITK 5.1.2 creates long long image (which subsequently
	// cannot be shown); so let's cast it to int:
	auto cast = castImageTo<int>(out);
	filter->addOutput(cast);
}

void iAConnectedComponents::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(connectedComponentFilter, inputScalarType(), this, parameters);
}

iAConnectedComponents::iAConnectedComponents() :
	iAFilter("Connected Component", "Connected Components",
		"Assigns each distinct object in a binary image a unique label.<br/>"
		"Non-zero pixels are considered to be objects, zero-valued pixels are "
		"considered to be background).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConnectedComponentImageFilter.html\">"
		"Connected Component Filter</a> in the ITK documentation.")
{
	addParameter("Fully Connected", iAValueType::Boolean, false);
}


template<class T>
void scalarConnectedComponentFilter(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::ScalarConnectedComponentImageFilter< InputImageType, OutputImageType > SCCIFType;
	typename SCCIFType::Pointer sccFilter = SCCIFType::New();
	sccFilter->SetInput( dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()) );
	sccFilter->SetDistanceThreshold(parameters["Distance Threshold"].toDouble());
	filter->progress()->observe(sccFilter);
	sccFilter->Update();
	filter->addOutput(sccFilter->GetOutput());
}

void iAScalarConnectedComponents::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(scalarConnectedComponentFilter, inputScalarType(), this, parameters);
}

iAScalarConnectedComponents::iAScalarConnectedComponents() :
	iAFilter("Scalar Connected Component", "Connected Components",
		"Labels the objects in an arbitrary image.<br/>"
		"Two pixels are similar if they are within <em>Distance Threshold</em> of each other.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ScalarConnectedComponentImageFilter.html\">"
		"Scalar Connected Component Filter</a> in the ITK documentation.")
{
	addParameter("Distance Threshold", iAValueType::Continuous, 1);
}


template<class T>
void relabelComponentImageFilter(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::RelabelComponentImageFilter< InputImageType, OutputImageType > RCIFType;
	typename RCIFType::Pointer rccFilter = RCIFType::New();
	rccFilter->SetInput( dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()) );
	rccFilter->SetMinimumObjectSize(parameters["Minimum object size"].toInt());
	filter->progress()->observe(rccFilter);
	rccFilter->Update();
	if (parameters["Write labels to file"].toBool())
	{
		long int no_of_Objects = rccFilter->GetNumberOfObjects();
		std::ofstream myfile;
		myfile.open(getLocalEncodingFileName(parameters["Label file"].toString()));
		myfile << " Total Objects " << "," << no_of_Objects << "\n";
		myfile << "Object Number" << "," << "Object Size (PhysicalUnits)\n";
		for (int i = 0; i < no_of_Objects; i++)
		{
			myfile << i << "," << rccFilter->GetSizeOfObjectsInPhysicalUnits()[i] << "\n";
		}
		myfile.close();
	}
	filter->addOutput(rccFilter->GetOutput());
}

void iARelabelComponents::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(relabelComponentImageFilter, inputScalarType(), this, parameters);
}

iARelabelComponents::iARelabelComponents() :
	iAFilter("Relabel Components", "Connected Components",
		"Remaps the labels associated with the objects in an image such that the "
		"label numbers are consecutive with no gaps.<br/>"
		"The input could for example be the output of the Connected "
		"Component Filter. By default, the relabeling will also sort the labels "
		"based on the size of the object: the largest object will have label #1, "
		"the second largest will have label #2, etc. If two labels have the same "
		"size, their initial order is kept. Label #0 is assumed to be the "
		"background and is left unaltered by the relabeling.<br/>"
		"If the user sets a <em>Minimum object size</em>, all objects with fewer pixels than "
		"the minimum will be discarded, so that the number of objects reported "
		"will be only those remaining. Enabling the option <em>Write labels to file</em> "
		"will save details of each object to the file specified under <em>Label file</em>.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RelabelComponentImageFilter.html\">"
		"Relabel Component Filter</a> in the ITK documentation.")
{
	addParameter("Minimum object size", iAValueType::Discrete, 1, 1);
	addParameter("Write labels to file", iAValueType::Boolean, false);
	addParameter("Label file", iAValueType::FileNameSave, ".csv");
}
