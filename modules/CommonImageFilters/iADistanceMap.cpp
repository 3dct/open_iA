// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iADataSet.h>
#include <iAFilterDefault.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>

IAFILTER_DEFAULT_CLASS(iASignedMaurerDistanceMap);
IAFILTER_DEFAULT_CLASS(iADanielssonDistanceMap);

template<class T>
void signed_maurer_distancemap(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, 3 > InputImageType;
	typedef itk::Image< float, 3 > RealImageType;
	typedef itk::SignedMaurerDistanceMapImageFilter< InputImageType, RealImageType > SDDMType;
	auto distFilter = SDDMType::New();
	distFilter->SetInput( dynamic_cast< InputImageType * >( filter->imageInput(0)->itkImage() ) );
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

void iASignedMaurerDistanceMap::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(signed_maurer_distancemap, inputScalarType(), this, parameters);
}

iASignedMaurerDistanceMap::iASignedMaurerDistanceMap() :
	iAFilter("Signed Maurer Distance Map", "Distance Map",
		"This filter calculates the Euclidean distance transform of a binary "
		"image in linear time for arbitrary dimensions.<br/>"
		"<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SignedMaurerDistanceMapImageFilter.html\">"
		"Signed Maurer Distance Map Filter</a> in the ITK documentation.")
{
	addParameter("Use image spacing", iAValueType::Boolean, true);
	addParameter("Squared distance", iAValueType::Boolean, false);
	addParameter("Inside positive", iAValueType::Boolean, false);
	addParameter("Remove negative values", iAValueType::Boolean, false);
	addParameter("Background Value", iAValueType::Continuous, 0);
}



template<class T>
void danielsson_distancemap(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< unsigned short, 3 >   UShortImageType;
	typedef itk::Image< unsigned char, 3 >   OutputImageType;
	typedef itk::DanielssonDistanceMapImageFilter< InputImageType, UShortImageType, UShortImageType > danielssonDistFilterType;

	auto distFilter = danielssonDistFilterType::New();
	distFilter->SetInputIsBinary(parameters["Input is binary"].toBool());
	distFilter->SetInput( dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage() ) );
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

void iADanielssonDistanceMap::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(danielsson_distancemap, inputScalarType(), this, parameters);
}

iADanielssonDistanceMap::iADanielssonDistanceMap() :
	iAFilter("Danielsson Distance Map", "Distance Map",
		"Computes the distance map of the input image as an approximation with "
		"pixel accuracy to the Euclidean distance. "
		"If <em>Input is binary</em> is set, each nonzero pixel in the input image will be "
		"given a unique numeric code to be used by the Voronoi partition. If the image is "
		"binary but you are not interested in the Voronoi regions of the different nonzero "
		"pixels, then you need not set this. <br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DanielssonDistanceMapImageFilter.html\">"
		"Danielsson Distance Map Filter</a> in the ITK documentation.")
{
	addParameter("Input is binary", iAValueType::Boolean, true);
	addParameter("Rescale to unsigned char", iAValueType::Boolean, false);
}
