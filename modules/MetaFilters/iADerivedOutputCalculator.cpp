// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADerivedOutputCalculator.h"

#include <iAAttributes.h>
#include <iAITKImageTypes.h>

#include "iASingleResult.h"

// Toolkit/Entropy
#include <iAEntropyImageFilter.h>

#include <iALog.h>
#include <iAToolsITK.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImageFileWriter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkStatisticsImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <QString>

iADerivedOutputCalculator::iADerivedOutputCalculator(
		std::shared_ptr<iASingleResult> result,
		int objCountIdx,
		int avgUncIdx,
		int labelCount):
	m_result(result),
	m_objCountIdx(objCountIdx),
	m_avgUncIdx(avgUncIdx),
	m_success(true),
	m_labelCount(labelCount)
{}

void iADerivedOutputCalculator::run()
{
	try
	{
		typedef itk::Image< unsigned int, 3 > OutputImageType;
		typedef itk::ScalarConnectedComponentImageFilter <LabelImageType, OutputImageType > ConnectedComponentImageFilterType;
		ConnectedComponentImageFilterType::Pointer connected = ConnectedComponentImageFilterType::New();
		connected->SetDistanceThreshold(0);
		if (m_result->labelImage().IsNull())
		{
			LOG(lvlError, "Labelled Image is null");
			m_success = false;
			return;
		}
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(m_result->labelImage().GetPointer());
		connected->SetInput(lblImg);
		connected->Update();
		m_result->discardDetails();
		typedef itk::RelabelComponentImageFilter <OutputImageType, OutputImageType >
			RelabelFilterType;
		RelabelFilterType::Pointer relabel = RelabelFilterType::New();
		relabel->SetInput(connected->GetOutput());
		//relabel->SetSortByObjectSize(false);
		relabel->Update();
		int objCount = relabel->GetNumberOfObjects();
		m_result->setAttribute(m_objCountIdx, objCount);

		if (m_result->probabilityAvailable())
		{
			//typedef itk::ImageRegionConstIterator<ProbabilityImageType> ConstDblIt;
			typedef iAEntropyImageFilter<ProbabilityImageType, ProbabilityImageType> EntropyFilter;
			auto entropyFilter = EntropyFilter::New();
			for (int i = 0; i < m_labelCount; ++i)
			{
				ProbabilityImageType* probImg = dynamic_cast<ProbabilityImageType*>(m_result->probabilityImg(i).GetPointer());
				entropyFilter->SetInput(i, probImg);
			}
			entropyFilter->SetNormalize(true);
			entropyFilter->Update();
			double avgEntropy;
			getStatistics(entropyFilter->GetOutput(), nullptr, nullptr, &avgEntropy);
			if (qIsInf(avgEntropy))
			{
				LOG(lvlWarn, "AverageEntropy was infinity! Setting to -1");
				avgEntropy = -1;
			}
			m_result->setAttribute(m_avgUncIdx, avgEntropy);
			m_result->discardProbability();
		}
	}
	catch (std::exception & e)
	{
		LOG(lvlError, QString("An exception occured while computing derived output: %1").arg(e.what()));
		m_success = false;
	}
	/*
	itk::ImageFileWriter<OutputImageType>::Pointer writer = itk::ImageFileWriter<OutputImageType>::New();
	writer->SetFileName( getLocalEncodingFileName(labelOutputFileName) );
	writer->SetUseCompression(true);
	writer->SetInput(relabel->GetOutput() );
	writer->Update();
	*/
}

bool iADerivedOutputCalculator::success()
{
	return m_success;
}
