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
#include "iAFreeBeamCalculation.h"

#include "defines.h"        // for DIM
#include "iAConnector.h"
#include "iAProgress.h"		
#include "iATypedCallHelper.h"

#include <itkExtractImageFilter.h>
#include <itkImageIOBase.h>
#include <itkImageLinearIteratorWithIndex.h>
#include <itkImageRegionIterator.h>
#include <itkImageSliceConstIteratorWithIndex.h>
#include <itkImageSliceIteratorWithIndex.h>
#include <itkStatisticsImageFilter.h>

#include <vtkImageData.h>

#include <qmath.h>
#include <QLocale>


template<class T> 
int freeBeamCalculation_template( double indexX, double indexY, double indexZ, double sizeX, double sizeY, double sizeZ, 
	bool manualMeanFreeBeamIntensity, int manualMeanFreeBeamIntensityValue, iAProgress* p, iAConnector* image  )
{
	double I0 = manualMeanFreeBeamIntensityValue;
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< double, DIM > OutputImageType;
	const OutputImageType::SpacingType& outputSspacing = dynamic_cast<InputImageType *>(image->GetITKImage())->GetSpacing();
	const OutputImageType::PointType& outputOrigin = dynamic_cast<InputImageType *>(image->GetITKImage())->GetOrigin();
	OutputImageType::RegionType outputRegion = dynamic_cast<InputImageType *>(image->GetITKImage())->GetLargestPossibleRegion();
	OutputImageType::Pointer outputImage = OutputImageType::New();
	outputImage->SetRegions(outputRegion);
	outputImage->SetSpacing(outputSspacing);
	outputImage->SetOrigin(outputOrigin);
	outputImage->Allocate();

	if (!manualMeanFreeBeamIntensity)
	{
		unsigned int projectionDirection = 2;
		unsigned int i, j;
		unsigned int direction[2];
		for (i = 0, j = 0; i < 3; ++i)
		{
			if (i != projectionDirection)
			{
				direction[j] = i;
				j++;
			}
		}

		typedef itk::ExtractImageFilter< InputImageType, InputImageType > EIFType;
		typename EIFType::Pointer roiFilter = EIFType::New();
		typename EIFType::InputImageRegionType::SizeType roiSize; roiSize[0] = sizeX; roiSize[1] = sizeY; roiSize[2] = sizeZ;
		typename EIFType::InputImageRegionType::IndexType roiIndex; roiIndex[0] = indexX; roiIndex[1] = indexY; roiIndex[2] = indexZ;
		typename EIFType::InputImageRegionType roiRegion; roiRegion.SetIndex(roiIndex); roiRegion.SetSize(roiSize);
		roiFilter->SetInput(dynamic_cast<InputImageType *>(image->GetITKImage()));
		roiFilter->SetExtractionRegion(roiRegion);
		p->Observe(roiFilter);
		roiFilter->Update();

		typedef itk::Image< T, 2 > ImageType2D;
		typename ImageType2D::RegionType roiSliceRegion;
		typename ImageType2D::RegionType::SizeType roiSliceSize;
		typename ImageType2D::RegionType::IndexType roiSliceIndex;
		typename InputImageType::RegionType requestedROIRegion = roiFilter->GetOutput()->GetRequestedRegion();
		roiSliceIndex[direction[0]] = requestedROIRegion.GetIndex()[direction[0]];
		roiSliceIndex[1 - direction[0]] = requestedROIRegion.GetIndex()[direction[1]];
		roiSliceSize[direction[0]] = requestedROIRegion.GetSize()[direction[0]];
		roiSliceSize[1 - direction[0]] = requestedROIRegion.GetSize()[direction[1]];
		roiSliceRegion.SetSize(roiSliceSize);
		roiSliceRegion.SetIndex(roiSliceIndex);
		typename ImageType2D::Pointer outputROISliceImage = ImageType2D::New();
		outputROISliceImage->SetRegions(roiSliceRegion);
		outputROISliceImage->Allocate();
		
		typedef itk::ImageLinearIteratorWithIndex< ImageType2D > LinearIteratorType;
		typedef itk::ImageSliceConstIteratorWithIndex< InputImageType > SliceConstIteratorType;
		typedef itk::ImageSliceIteratorWithIndex< OutputImageType > SliceIteratorType;
		SliceConstIteratorType inputROIIt(roiFilter->GetOutput(), roiFilter->GetOutput()->GetRequestedRegion());
		LinearIteratorType outputROISliceIt(outputROISliceImage, outputROISliceImage->GetRequestedRegion());
		inputROIIt.SetFirstDirection(direction[1]);
		inputROIIt.SetSecondDirection(direction[0]);
		outputROISliceIt.SetDirection(1 - direction[0]);
		SliceConstIteratorType inputIt(dynamic_cast<InputImageType *>(image->GetITKImage()),
			dynamic_cast<InputImageType *>(image->GetITKImage())->GetLargestPossibleRegion());
		SliceIteratorType outputIt(outputImage, outputImage->GetLargestPossibleRegion());
		inputIt.SetFirstDirection(direction[1]);
		inputIt.SetSecondDirection(direction[0]);
		outputIt.SetFirstDirection(direction[1]);
		outputIt.SetSecondDirection(direction[0]);

		inputROIIt.GoToBegin();
		outputROISliceIt.GoToBegin();
		inputIt.GoToBegin();
		outputIt.GoToBegin();
		while (!inputROIIt.IsAtEnd())
		{
			while (!inputROIIt.IsAtEndOfSlice())
			{
				while (!inputROIIt.IsAtEndOfLine())
				{
					outputROISliceIt.Set(inputROIIt.Get());
					++inputROIIt;
					++outputROISliceIt;
				}
				outputROISliceIt.NextLine();
				inputROIIt.NextLine();
			}

			typedef itk::StatisticsImageFilter<ImageType2D> StatisticsImageFilterType;
			typename StatisticsImageFilterType::Pointer statisticsImageFilter = StatisticsImageFilterType::New();
			statisticsImageFilter->SetInput(outputROISliceImage);
			statisticsImageFilter->Update();
			I0 = statisticsImageFilter->GetMean();

			while (!inputIt.IsAtEndOfSlice())
			{
				while (!inputIt.IsAtEndOfLine())
				{
					double I = inputIt.Get();
					double res = (-1.0)*qLn(I / I0);
					outputIt.Set(res);
					++inputIt;
					++outputIt;
				}
				outputIt.NextLine();
				inputIt.NextLine();
			}

			outputIt.NextSlice();
			inputIt.NextSlice();
			outputROISliceIt.GoToBegin();
			inputROIIt.NextSlice();
		}
		roiFilter->ReleaseDataFlagOn();
	}
	else
	{
		typedef itk::ImageRegionConstIterator< InputImageType > InputIteratorType;
		typedef itk::ImageRegionIterator< OutputImageType > OutputIteratorType;
		typename InputIteratorType inputIt(dynamic_cast<InputImageType *>(image->GetITKImage()),
			dynamic_cast<InputImageType *>(image->GetITKImage())->GetLargestPossibleRegion());
		typename OutputIteratorType outputIt(outputImage, outputRegion);
		
		inputIt.GoToBegin();
		outputIt.GoToBegin();
		while (!inputIt.IsAtEnd())
		{
			double I = inputIt.Get();
			double res = (-1.0)*qLn(I / I0);
			outputIt.Set(res);
			++inputIt;
			++outputIt;
		}
	}

	image->SetImage(outputImage);
	image->Modified();
	return EXIT_SUCCESS;
}

iAFreeBeamCalculation::iAFreeBeamCalculation( QString fn, iAFreeBeamCalculationType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent  )
	: iAAlgorithm( fn, i, p, logger, parent ), m_operation(fid)
{
}

void iAFreeBeamCalculation::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	switch (m_operation)
	{
	case FREEBEAMCALCULATION:
		ITK_TYPED_CALL(freeBeamCalculation_template, itkType,
			originX, originY, originZ, sizeX, sizeY, sizeZ, manualMeanFreeBeamIntensity, manualMeanFreeBeamIntensityValue, getItkProgress(), getConnector());
		break;
	default:
		addMsg(tr("  unknown filter type"));
	}
}