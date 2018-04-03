/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <qmath.h>


template<class InPixelType, class OutPixelType>
void freeBeamCalculation(QMap<QString, QVariant> const & params, iAFilter* filter )
{
	double I0 = params["Manual I0"].toDouble();
	typedef itk::Image< InPixelType, DIM > InputImageType;
	typedef itk::Image< OutPixelType, DIM > OutputImageType;
	const typename OutputImageType::SpacingType& outputSpacing = dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage())->GetSpacing();
	const typename OutputImageType::PointType& outputOrigin = dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage())->GetOrigin();
	typename OutputImageType::RegionType outputRegion = dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage())->GetLargestPossibleRegion();
	typename OutputImageType::Pointer outputImage = OutputImageType::New();
	outputImage->SetRegions(outputRegion);
	outputImage->SetSpacing(outputSpacing);
	outputImage->SetOrigin(outputOrigin);
	outputImage->Allocate();

	if (!params["Set I0 manually"].toBool())
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
		typename EIFType::InputImageRegionType::SizeType roiSize;
		roiSize[0] = params["Size X"].toUInt();
		roiSize[1] = params["Size Y"].toUInt();
		roiSize[2] =  filter->Input()[0]->GetITKImage()->GetLargestPossibleRegion().GetSize()[2];
		typename EIFType::InputImageRegionType::IndexType roiIndex;
		roiIndex[0] = params["Index X"].toUInt(); roiIndex[1] = params["Index Y"].toUInt(); roiIndex[2] = 0;
		typename EIFType::InputImageRegionType roiRegion; roiRegion.SetIndex(roiIndex); roiRegion.SetSize(roiSize);
		roiFilter->SetInput(dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()));
		roiFilter->SetExtractionRegion(roiRegion);
		roiFilter->Update();

		typedef itk::Image< InPixelType, 2 > ImageType2D;
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
		SliceConstIteratorType inputIt(dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()),
			dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage())->GetLargestPossibleRegion());
		SliceIteratorType outputIt(outputImage, outputImage->GetLargestPossibleRegion());
		inputIt.SetFirstDirection(direction[1]);
		inputIt.SetSecondDirection(direction[0]);
		outputIt.SetFirstDirection(direction[1]);
		outputIt.SetSecondDirection(direction[0]);

		inputROIIt.GoToBegin();
		outputROISliceIt.GoToBegin();
		inputIt.GoToBegin();
		outputIt.GoToBegin();
		size_t curSlice = 0;
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
			++curSlice;
			filter->Progress()->EmitProgress(static_cast<int>((100.0 * curSlice) / roiSize[2]));
		}
		roiFilter->ReleaseDataFlagOn();
	}
	else
	{
		auto img = dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage());
		typedef itk::ImageRegionConstIterator< InputImageType > InputIteratorType;
		typedef itk::ImageRegionIterator< OutputImageType > OutputIteratorType;
		InputIteratorType inputIt(img, img->GetLargestPossibleRegion());
		OutputIteratorType outputIt(outputImage, outputRegion);
		
		inputIt.GoToBegin();
		outputIt.GoToBegin();
		auto size = img->GetLargestPossibleRegion().GetSize();
		size_t voxelCount = size[0] * size[1] * size[2];
		size_t progressVoxelDist = voxelCount / 100;
		size_t curVoxel = 0;
		size_t lastProgressReportVoxel = 0;
		while (!inputIt.IsAtEnd())
		{
			double I = inputIt.Get();
			double res = (-1.0)*qLn(I / I0);
			outputIt.Set(res);
			++inputIt;
			++outputIt;
			++curVoxel;
			if (curVoxel > (lastProgressReportVoxel + progressVoxelDist))
			{
				filter->Progress()->EmitProgress(static_cast<int>((100.0 * curVoxel) / voxelCount));
				lastProgressReportVoxel = curVoxel;
			}
		}
	}
	filter->AddOutput(outputImage);
}

template<class InPixelType>
void freeBeamCalculation_OutType(QMap<QString, QVariant> const & parameters, iAFilter* filter)
{
	if (parameters["Float output"].toBool())
		freeBeamCalculation<InPixelType, float>(parameters, filter);
	else
		freeBeamCalculation<InPixelType, double>(parameters, filter);
}

void iAFreeBeamCalculation::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(freeBeamCalculation_OutType, InputPixelType(), parameters, this);
}

IAFILTER_CREATE(iAFreeBeamCalculation)

iAFreeBeamCalculation::iAFreeBeamCalculation() :
	iAFilter("Free Beam Intensity", "Reconstruction",
		"Convert the intensity values to attenuation values via free beam intensity transform.<br/>"
		"You can specify the mean intensity I0 of the surrounding air manually. "
		"The given <em>Manual I0</em> is considered when <em>Set I0 manually</em> "
		"is checked.<br/>"
		"Or you can specify the region for each projection image (assumed to be in "
		"the XY plane) where there is just air; the mean intensity in that region "
		"will be taken as I0 image (separately for each projection image)."
		"Both <em>Index</em> and <em>Size</em> values are in pixel units.<br/>"
		"If <em>Float output</em> is enabled, then the output will be in float "
		"datatype, otherwise double will be used.")
{
	AddParameter("Index X", Discrete, 0);
	AddParameter("Index Y", Discrete, 0);
	AddParameter("Size X", Discrete, 1, 1);
	AddParameter("Size Y", Discrete, 1, 1);
	AddParameter("Set I0 manually", Boolean, false);
	AddParameter("Manual I0", Continuous, 0);
	AddParameter("Float output", Boolean, true);
}
