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
#include "iARegionGrowing.h"

#include <defines.h>          // for DIM
#include <iADataSet.h>
#include <iALog.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkConfidenceConnectedImageFilter.h>
#include <itkConnectedThresholdImageFilter.h>
#include <itkImageRegionConstIteratorWithIndex.h>
//#include <itkIsolatedConnectedImageFilter.h>
//#include <itkKLMRegionGrowImageFilter.h>
#include <itkNeighborhoodConnectedImageFilter.h>
//#include <itkVectorConfidenceConnectedImageFilter.h>


// Common functionality for adding seeds from a given mask image:
namespace
{
template <typename RegFilterT, typename MaskImageT>
void addSeeds(RegFilterT* regionGrowingFilter, MaskImageT * seed)
{
	itk::ImageRegionConstIteratorWithIndex<MaskImageT> imageIterator(seed, seed->GetLargestPossibleRegion());
	while (!imageIterator.IsAtEnd())
	{
		if (imageIterator.Get() == 1)
			regionGrowingFilter->AddSeed(imageIterator.GetIndex());

		++imageIterator;
	}
}

template <typename RegFilterT>
void setSeeds(RegFilterT* filter, itk::ImageBase<3>* img)
{
	switch (itkScalarType(img))
	{
		case iAITKIO::ScalarType::CHAR:   addSeeds(filter, dynamic_cast<itk::Image<          char, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::UCHAR:  addSeeds(filter, dynamic_cast<itk::Image< unsigned char, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::SHORT:  addSeeds(filter, dynamic_cast<itk::Image<         short, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::USHORT: addSeeds(filter, dynamic_cast<itk::Image<unsigned short, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::INT:    addSeeds(filter, dynamic_cast<itk::Image<           int, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::UINT:   addSeeds(filter, dynamic_cast<itk::Image<  unsigned int, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::LONG:   addSeeds(filter, dynamic_cast<itk::Image<          long, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::ULONG:  addSeeds(filter, dynamic_cast<itk::Image< unsigned long, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::LONGLONG:  addSeeds(filter, dynamic_cast<itk::Image<  long long, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::ULONGLONG: addSeeds(filter, dynamic_cast<itk::Image<unsigned long long, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::FLOAT:  addSeeds(filter, dynamic_cast<itk::Image<         float, iAITKIO::Dim>*>(img)); break;
		case iAITKIO::ScalarType::DOUBLE: addSeeds(filter, dynamic_cast<itk::Image<        double, iAITKIO::Dim>*>(img)); break;
		default:
			LOG(lvlError, "Invalid/Unknown itk pixel datatype in setSeeds!"); break;
	}
}
}



template<class T>
void confidenceConnected(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage());

	typedef itk::ConfidenceConnectedImageFilter< InputImageType, InputImageType > ConfiConnFilterType;
	auto confiConnFilter = ConfiConnFilterType::New();
	confiConnFilter->SetInput(input);
	confiConnFilter->SetInitialNeighborhoodRadius(params["Initial neighborhood radius"].toUInt());
	confiConnFilter->SetMultiplier(params["Multiplier"].toDouble());
	confiConnFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	confiConnFilter->SetReplaceValue(params["Replace value"].toDouble());
	setSeeds(confiConnFilter.GetPointer(), filter->imageInput(1)->itkImage());

	confiConnFilter->ReleaseDataFlagOn();
	confiConnFilter->Update();
	filter->addOutput(confiConnFilter->GetOutput());
}

void iAConfidenceConnectedRegionGrow::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(confidenceConnected, inputScalarType(), this, parameters);
}

iAConfidenceConnectedRegionGrow::iAConfidenceConnectedRegionGrow() :
	iAFilter("Confidence Connected", "Segmentation/Region-Growing",
		"Segment pixels with similar statistics using connectivity.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConfidenceConnectedImageFilter.html\">"
		"Confidence Connected Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Initial neighborhood radius", iAValueType::Discrete, 1, 1);
	addParameter("Multiplier", iAValueType::Continuous, 2.5, std::numeric_limits<double>::epsilon()); // needs to be bigger than 0
	addParameter("Number of iterations", iAValueType::Discrete, 100, 1);
	addParameter("Replace value", iAValueType::Continuous, 1.0);
	setInputName(1u, "Mask image");
}



template<class T>
void connectedThreshold(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	const InputImageType * input = dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage());

	typedef itk::ConnectedThresholdImageFilter< InputImageType, InputImageType > ConnThrFilterType;
	auto connThrfilter = ConnThrFilterType::New();
	connThrfilter->SetInput(input);
	connThrfilter->SetLower(params["Lower connection threshold"].toDouble());
	connThrfilter->SetUpper(params["Upper connection threshold"].toDouble());
	connThrfilter->SetReplaceValue(params["Replace value"].toDouble());
	setSeeds(connThrfilter.GetPointer(), filter->imageInput(1)->itkImage());
	connThrfilter->ReleaseDataFlagOn();
	connThrfilter->Update();
	filter->addOutput(connThrfilter->GetOutput());
}

void iAConnectedThresholdRegionGrow::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(connectedThreshold, inputScalarType(), this, parameters);
}

iAConnectedThresholdRegionGrow::iAConnectedThresholdRegionGrow() :
	iAFilter("Connected Threshold", "Segmentation/Region-Growing",
		"Label pixels that are connected to a seed and lie within a range of values.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConnectedThresholdImageFilter.html\">"
		"Connected Threshold Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Lower connection threshold", iAValueType::Continuous, 0.0);
	addParameter("Upper connection threshold", iAValueType::Continuous, 0.0);
	addParameter("Replace value", iAValueType::Continuous, 1.0);
	setInputName(1u, "Mask image");
}



template<class T>
void neighborhoodConnected(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, iAITKIO::Dim>   InputImageType;
	const InputImageType * input = dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage());

	typename InputImageType::SizeType	radius;
	radius[0] = params["Neighborhood radius"].toUInt();
	radius[1] = params["Neighborhood radius"].toUInt();
	radius[2] = params["Neighborhood radius"].toUInt();

	typedef itk::NeighborhoodConnectedImageFilter< InputImageType, InputImageType > NeighbConnFilterType;
	auto neighbConnfilter = NeighbConnFilterType::New();
	neighbConnfilter->SetInput(input);
	neighbConnfilter->SetLower(params["Lower connection threshold"].toDouble());
	neighbConnfilter->SetUpper(params["Upper connection threshold"].toDouble());
	neighbConnfilter->SetRadius(radius);
	neighbConnfilter->SetReplaceValue(params["Replace value"].toDouble());
	setSeeds(neighbConnfilter.GetPointer(), filter->imageInput(1)->itkImage());
	neighbConnfilter->ReleaseDataFlagOn();
	neighbConnfilter->Update();
	filter->addOutput(neighbConnfilter->GetOutput());
}

void iANeighborhoodConnectedRegionGrow::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(neighborhoodConnected, inputScalarType(), this, parameters);
}

iANeighborhoodConnectedRegionGrow::iANeighborhoodConnectedRegionGrow() :
	iAFilter("Neighborhood Connected", "Segmentation/Region-Growing",
		"Label pixels that are connected to a seed and lie within a neighborhood. .<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NeighborhoodConnectedImageFilter.html\">"
		"Neighborhood Connected Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Neighborhood radius", iAValueType::Discrete, 1, 1);
	addParameter("Lower connection threshold", iAValueType::Continuous, 0.0);
	addParameter("Upper connection threshold", iAValueType::Continuous, 0.0);
	addParameter("Replace value", iAValueType::Continuous, 1.0);
	setInputName(1u, "Mask image");
}



// itk::IsolatedConnectedImageFilter
// itk::KLMRegionGrowImageFilter
// itk::VectorConfidenceConnectedImageFilter