// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>        // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>
#include <iAValueTypeVectorHelpers.h>

#include <itkExtractImageFilter.h>
#include <itkImageLinearIteratorWithIndex.h>
#include <itkImageRegionIterator.h>
#include <itkImageSliceConstIteratorWithIndex.h>
#include <itkImageSliceIteratorWithIndex.h>
#include <itkStatisticsImageFilter.h>

#include <QtMath>

IAFILTER_DEFAULT_CLASS(iAFreeBeamCalculation);

template<class InPixelType, class OutPixelType>
void freeBeamCalculation(QVariantMap const & params, iAFilter* filter )
{
	double I0 = params["Manual I0"].toDouble();
	using InputImageType = itk::Image<InPixelType, DIM>;
	using OutputImageType = itk::Image<OutPixelType, DIM>;
	auto itkImg = dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage());
	auto outputSpacing = itkImg->GetSpacing();
	auto outputOrigin = itkImg->GetOrigin();
	auto outputRegion = itkImg->GetLargestPossibleRegion();
	auto outputImage = OutputImageType::New();
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

		using EIFType = itk::ExtractImageFilter<InputImageType, InputImageType>;
		auto roiFilter = EIFType::New();
		typename EIFType::InputImageRegionType::SizeType roiSize;
		setFromVectorVariant<int>(roiSize, params["Size"]);
		roiSize[2] = outputRegion.GetSize()[2];
		typename EIFType::InputImageRegionType::IndexType roiIndex;
		setFromVectorVariant<int>(roiIndex, params["Index"]);
		roiIndex[2] = 0;
		typename EIFType::InputImageRegionType roiRegion; roiRegion.SetIndex(roiIndex); roiRegion.SetSize(roiSize);
		roiFilter->SetInput(itkImg);
		roiFilter->SetExtractionRegion(roiRegion);
		roiFilter->Update();

		using ImageType2D = itk::Image<InPixelType, 2>;
		typename ImageType2D::RegionType roiSliceRegion;
		typename ImageType2D::RegionType::SizeType roiSliceSize;
		typename ImageType2D::RegionType::IndexType roiSliceIndex;
		auto requestedROIRegion = roiFilter->GetOutput()->GetRequestedRegion();
		roiSliceIndex[direction[0]] = requestedROIRegion.GetIndex()[direction[0]];
		roiSliceIndex[1 - direction[0]] = requestedROIRegion.GetIndex()[direction[1]];
		roiSliceSize[direction[0]] = requestedROIRegion.GetSize()[direction[0]];
		roiSliceSize[1 - direction[0]] = requestedROIRegion.GetSize()[direction[1]];
		roiSliceRegion.SetSize(roiSliceSize);
		roiSliceRegion.SetIndex(roiSliceIndex);
		auto outputROISliceImage = ImageType2D::New();
		outputROISliceImage->SetRegions(roiSliceRegion);
		outputROISliceImage->Allocate();

		using SliceConstIteratorType = itk::ImageSliceConstIteratorWithIndex<InputImageType>;
		SliceConstIteratorType inputROIIt(roiFilter->GetOutput(), roiFilter->GetOutput()->GetRequestedRegion());
		itk::ImageLinearIteratorWithIndex<ImageType2D> outputROISliceIt(outputROISliceImage, outputROISliceImage->GetRequestedRegion());
		inputROIIt.SetFirstDirection(direction[1]);
		inputROIIt.SetSecondDirection(direction[0]);
		outputROISliceIt.SetDirection(1 - direction[0]);
		SliceConstIteratorType inputIt(itkImg, itkImg->GetLargestPossibleRegion());
		itk::ImageSliceIteratorWithIndex<OutputImageType> outputIt(outputImage, outputImage->GetLargestPossibleRegion());
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

			auto statisticsImageFilter = itk::StatisticsImageFilter<ImageType2D>::New();
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
			filter->progress()->emitProgress(curSlice * 100.0 / roiSize[2]);
		}
		roiFilter->ReleaseDataFlagOn();
	}
	else
	{
		itk::ImageRegionConstIterator<InputImageType> inputIt(itkImg, itkImg->GetLargestPossibleRegion());
		itk::ImageRegionIterator<OutputImageType> outputIt(outputImage, outputRegion);

		inputIt.GoToBegin();
		outputIt.GoToBegin();
		auto size = itkImg->GetLargestPossibleRegion().GetSize();
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
				filter->progress()->emitProgress(curVoxel * 100.0 / voxelCount);
				lastProgressReportVoxel = curVoxel;
			}
		}
	}
	filter->addOutput(std::make_shared<iAImageData>(outputImage));
}

template<class InPixelType>
void freeBeamCalculation_OutType(QVariantMap const & parameters, iAFilter* filter)
{
	if (parameters["Float output"].toBool())
	{
		freeBeamCalculation<InPixelType, float>(parameters, filter);
	}
	else
	{
		freeBeamCalculation<InPixelType, double>(parameters, filter);
	}
}

void iAFreeBeamCalculation::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(freeBeamCalculation_OutType, inputScalarType(), parameters, this);
}

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
	addParameter("Index", iAValueType::Vector2i, variantVector<int>({0, 0}));
	addParameter("Size", iAValueType::Vector2i, variantVector<int>({1, 1}));
	addParameter("Set I0 manually", iAValueType::Boolean, false);
	addParameter("Manual I0", iAValueType::Continuous, 0);
	addParameter("Float output", iAValueType::Boolean, true);
}
