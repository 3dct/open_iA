/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iADistanceMap.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>


template<class T> 
void signed_maurer_distancemap_template(iAProgress* p, iAConnector* image, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, 3 > InputImageType;
	typedef itk::Image< float, 3 > RealImageType;
	typedef itk::SignedMaurerDistanceMapImageFilter< InputImageType, RealImageType > SDDMType;
	auto filter = SDDMType::New();
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetBackgroundValue(parameters["Background Value"].toDouble());
	filter->SetUseImageSpacing(parameters["Use image spacing"].toBool());
	filter->SetSquaredDistance(parameters["Squared distance"].toBool());
	filter->SetInsideIsPositive(parameters["Inside positive"].toBool());
	p->Observe(filter);
	filter->Update();
	auto distanceImage = filter->GetOutput();
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
	image->SetImage( distanceImage );
	image->Modified();
	filter->ReleaseDataFlagOn();
}

void iASignedMaurerDistanceMap::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(signed_maurer_distancemap_template, Input()[0]->GetITKScalarPixelType(),
			m_progress, Input()[0], parameters);
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
	AddParameter("Use image spacing", Boolean, true);
	AddParameter("Squared distance", Boolean, false);
	AddParameter("Inside positive", Boolean, false);
	AddParameter("Remove negative values", Boolean, false);
	AddParameter("Background Value", Continuous, 0);
}



template<class T>
void danielsson_distancemap_template(iAProgress* p, iAConnector* image, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< unsigned short, 3 >   UShortImageType;
	typedef itk::Image< unsigned char, 3 >   OutputImageType;
	typedef itk::DanielssonDistanceMapImageFilter< InputImageType, UShortImageType, UShortImageType > danielssonDistFilterType;

	auto filter = danielssonDistFilterType::New();
	filter->SetInputIsBinary(parameters["Input binary"].toBool());
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	p->Observe( filter );
	filter->Update();

	if (!parameters["Rescale to unsigned char"].toBool())
	{
		image->SetImage(filter->GetOutput());
		image->Modified();
	}
	else
	{
		typedef itk::RescaleIntensityImageFilter< UShortImageType, OutputImageType > RescaleFilterType;
		auto intensityRescaler = RescaleFilterType::New();
		intensityRescaler->SetInput( filter->GetOutput() );
		intensityRescaler->SetOutputMinimum( 0 );
		intensityRescaler->SetOutputMaximum( 255 );
		intensityRescaler->Update();
		image->SetImage( intensityRescaler->GetOutput() );
		image->Modified();
		intensityRescaler->ReleaseDataFlagOn();
	}
	filter->ReleaseDataFlagOn();
}

void iADanielssonDistanceMap::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(danielsson_distancemap_template, m_con->GetITKScalarPixelType(),
			m_progress, m_con, parameters);
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
	AddParameter("Input binary", Boolean, true);
	AddParameter("Rescale to unsigned char", Boolean, false);
}
