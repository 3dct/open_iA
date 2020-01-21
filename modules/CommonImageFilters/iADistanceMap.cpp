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
#include "iADistanceMap.h"

#include <iAConnector.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>


template<class T>
void signed_maurer_distancemap(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, 3 > InputImageType;
	typedef itk::Image< float, 3 > RealImageType;
	typedef itk::SignedMaurerDistanceMapImageFilter< InputImageType, RealImageType > SDDMType;
	auto distFilter = SDDMType::New();
	distFilter->SetInput( dynamic_cast< InputImageType * >( filter->input()[0]->itkImage() ) );
	distFilter->SetBackgroundValue(parameters["Background Value"].toDouble());
	distFilter->SetUseImageSpacing(parameters["Use image spacing"].toBool());
	distFilter->SetSquaredDistance(parameters["Squared distance"].toBool());
	distFilter->SetInsideIsPositive(parameters["Inside positive"].toBool());
	filter->progress()->observe(distFilter);
	distFilter->Update();
	auto distanceImage = distFilter->GetOutput();
	if (parameters["Remove negative values"].toBool())
	{
		typedef itk::ImageRegionIterator<RealImageType> ImageIteratorType;
		ImageIteratorType iter ( distanceImage, distanceImage->GetLargestPossibleRegion() );
		iter.GoToBegin();
		while (!iter.IsAtEnd() )
		{
			if (iter.Get() < 0 )
				iter.Set(-1);
			++iter;
		}
	}
	filter->addOutput( distanceImage );
}

void iASignedMaurerDistanceMap::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(signed_maurer_distancemap, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iASignedMaurerDistanceMap)

iASignedMaurerDistanceMap::iASignedMaurerDistanceMap() :
	iAFilter("Signed Maurer Distance Map", "Distance Map",
		"This filter calculates the Euclidean distance transform of a binary "
		"image in linear time for arbitrary dimensions.<br/>"
		"<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SignedMaurerDistanceMapImageFilter.html\">"
		"Signed Maurer Distance Map Filter</a> in the ITK documentation.")
{
	addParameter("Use image spacing", Boolean, true);
	addParameter("Squared distance", Boolean, false);
	addParameter("Inside positive", Boolean, false);
	addParameter("Remove negative values", Boolean, false);
	addParameter("Background Value", Continuous, 0);
}



template<class T>
void danielsson_distancemap(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< unsigned short, 3 >   UShortImageType;
	typedef itk::Image< unsigned char, 3 >   OutputImageType;
	typedef itk::DanielssonDistanceMapImageFilter< InputImageType, UShortImageType, UShortImageType > danielssonDistFilterType;

	auto distFilter = danielssonDistFilterType::New();
	distFilter->SetInputIsBinary(parameters["Input binary"].toBool());
	distFilter->SetInput( dynamic_cast< InputImageType * >(filter->input()[0]->itkImage() ) );
	filter->progress()->observe(distFilter);
	distFilter->Update();

	if (!parameters["Rescale to unsigned char"].toBool())
	{
		filter->addOutput(distFilter->GetOutput());
	}
	else
	{
		typedef itk::RescaleIntensityImageFilter< UShortImageType, OutputImageType > RescaleFilterType;
		auto intensityRescaler = RescaleFilterType::New();
		intensityRescaler->SetInput( distFilter->GetOutput() );
		intensityRescaler->SetOutputMinimum( 0 );
		intensityRescaler->SetOutputMaximum( 255 );
		intensityRescaler->Update();
		filter->addOutput( intensityRescaler->GetOutput() );
	}
}

void iADanielssonDistanceMap::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(danielsson_distancemap, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iADanielssonDistanceMap)

iADanielssonDistanceMap::iADanielssonDistanceMap() :
	iAFilter("Danielsson Distance Map", "Distance Map",
		"Computes the distance map of the input image as an approximation with "
		"pixel accuracy to the Euclidean distance. <br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DanielssonDistanceMapImageFilter.html\">"
		"Danielsson Distance Map Filter</a> in the ITK documentation.")
{
	addParameter("Input binary", Boolean, true);
	addParameter("Rescale to unsigned char", Boolean, false);
}
