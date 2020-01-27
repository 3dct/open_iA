/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iARegionGrowing.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAConsole.h>
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
void setSeeds(RegFilterT* filter, iAConnector::ImageBaseType* img)
{
	switch (itkScalarPixelType(img))
	{
		case itk::ImageIOBase::CHAR:   addSeeds(filter, dynamic_cast<itk::Image<          char, DIM>*>(img)); break;
		case itk::ImageIOBase::UCHAR:  addSeeds(filter, dynamic_cast<itk::Image< unsigned char, DIM>*>(img)); break;
		case itk::ImageIOBase::SHORT:  addSeeds(filter, dynamic_cast<itk::Image<         short, DIM>*>(img)); break;
		case itk::ImageIOBase::USHORT: addSeeds(filter, dynamic_cast<itk::Image<unsigned short, DIM>*>(img)); break;
		case itk::ImageIOBase::INT:    addSeeds(filter, dynamic_cast<itk::Image<           int, DIM>*>(img)); break;
		case itk::ImageIOBase::UINT:   addSeeds(filter, dynamic_cast<itk::Image<  unsigned int, DIM>*>(img)); break;
		case itk::ImageIOBase::LONG:   addSeeds(filter, dynamic_cast<itk::Image<          long, DIM>*>(img)); break;
		case itk::ImageIOBase::ULONG:  addSeeds(filter, dynamic_cast<itk::Image< unsigned long, DIM>*>(img)); break;
#if ITK_VERSION_MAJOR > 4 || (ITK_VERSION_MAJOR == 4 && ITK_VERSION_MINOR > 12)
		case itk::ImageIOBase::LONGLONG:  addSeeds(filter, dynamic_cast<itk::Image<  long long, DIM>*>(img)); break;
		case itk::ImageIOBase::ULONGLONG: addSeeds(filter, dynamic_cast<itk::Image<unsigned long long, DIM>*>(img)); break;
#endif
		case itk::ImageIOBase::FLOAT:  addSeeds(filter, dynamic_cast<itk::Image<         float, DIM>*>(img)); break;
		case itk::ImageIOBase::DOUBLE: addSeeds(filter, dynamic_cast<itk::Image<        double, DIM>*>(img)); break;
		default:
			DEBUG_LOG("ERROR: Invalid/Unknown itk pixel datatype in setSeeds!"); break;
	}
}
}



template<class T>
void confidenceConnected(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType *>(filter->input()[0]->itkImage());

	typedef itk::ConfidenceConnectedImageFilter< InputImageType, InputImageType > ConfiConnFilterType;
	auto confiConnFilter = ConfiConnFilterType::New();
	confiConnFilter->SetInput(input);
	confiConnFilter->SetInitialNeighborhoodRadius(params["Initial neighborhood radius"].toUInt());
	confiConnFilter->SetMultiplier(params["Multiplier"].toDouble());
	confiConnFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	confiConnFilter->SetReplaceValue(params["Replace value"].toDouble());
	setSeeds(confiConnFilter.GetPointer(), filter->input()[1]->itkImage());

	confiConnFilter->ReleaseDataFlagOn();
	confiConnFilter->Update();
	filter->addOutput(confiConnFilter->GetOutput());
}

IAFILTER_CREATE(iAConfidenceConnectedRegionGrow)

void iAConfidenceConnectedRegionGrow::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(confidenceConnected, inputPixelType(), this, parameters);
}

iAConfidenceConnectedRegionGrow::iAConfidenceConnectedRegionGrow() :
	iAFilter("Confidence Connected", "Segmentation/Region-Growing",
		"Segment pixels with similar statistics using connectivity.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConfidenceConnectedImageFilter.html\">"
		"Confidence Connected Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Initial neighborhood radius", Discrete, 1, 1);
	addParameter("Multiplier", Continuous, 2.5, std::numeric_limits<double>::epsilon()); // needs to be bigger than 0
	addParameter("Number of iterations", Discrete, 100, 1);
	addParameter("Replace value", Continuous, 1.0);
	setInputName(1, "Mask image");
}



template<class T>
void connectedThreshold(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	const InputImageType * input = dynamic_cast<InputImageType*>(filter->input()[0]->itkImage());

	typedef itk::ConnectedThresholdImageFilter< InputImageType, InputImageType > ConnThrFilterType;
	auto connThrfilter = ConnThrFilterType::New();
	connThrfilter->SetInput(input);
	connThrfilter->SetLower(params["Lower connection threshold"].toDouble());
	connThrfilter->SetUpper(params["Upper connection threshold"].toDouble());
	connThrfilter->SetReplaceValue(params["Replace value"].toDouble());
	setSeeds(connThrfilter.GetPointer(), filter->input()[1]->itkImage());
	connThrfilter->ReleaseDataFlagOn();
	connThrfilter->Update();
	filter->addOutput(connThrfilter->GetOutput());
}

IAFILTER_CREATE(iAConnectedThresholdRegionGrow)

void iAConnectedThresholdRegionGrow::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(connectedThreshold, inputPixelType(), this, parameters);
}

iAConnectedThresholdRegionGrow::iAConnectedThresholdRegionGrow() :
	iAFilter("Connected Threshold", "Segmentation/Region-Growing",
		"Label pixels that are connected to a seed and lie within a range of values.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConnectedThresholdImageFilter.html\">"
		"Connected Threshold Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Lower connection threshold", Continuous, 0.0);
	addParameter("Upper connection threshold", Continuous, 0.0);
	addParameter("Replace value", Continuous, 1.0);
	setInputName(1, "Mask image");
}



template<class T>
void neighborhoodConnected(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	const InputImageType * input = dynamic_cast<InputImageType*>(filter->input()[0]->itkImage());

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
	setSeeds(neighbConnfilter.GetPointer(), filter->input()[1]->itkImage());
	neighbConnfilter->ReleaseDataFlagOn();
	neighbConnfilter->Update();
	filter->addOutput(neighbConnfilter->GetOutput());
}

IAFILTER_CREATE(iANeighborhoodConnectedRegionGrow)

void iANeighborhoodConnectedRegionGrow::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(neighborhoodConnected, inputPixelType(), this, parameters);
}

iANeighborhoodConnectedRegionGrow::iANeighborhoodConnectedRegionGrow() :
	iAFilter("Neighborhood Connected", "Segmentation/Region-Growing",
		"Label pixels that are connected to a seed and lie within a neighborhood. .<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NeighborhoodConnectedImageFilter.html\">"
		"Neighborhood Connected Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Neighborhood radius", Discrete, 1, 1);
	addParameter("Lower connection threshold", Continuous, 0.0);
	addParameter("Upper connection threshold", Continuous, 0.0);
	addParameter("Replace value", Continuous, 1.0);
	setInputName(1, "Mask image");
}



// itk::IsolatedConnectedImageFilter
// itk::KLMRegionGrowImageFilter
// itk::VectorConfidenceConnectedImageFilter