/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#include "iADerivedOutputCalculator.h"

#include "EntropyImageFilter.h"
#include "iAAttributes.h"
#include "iAConsole.h"
#include "iAImageTreeNode.h"
#include "iASingleResult.h"
#include "iAToolsITK.h"

#include <itkImageFileWriter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkStatisticsImageFilter.h>

#include <QString>

iADerivedOutputCalculator::iADerivedOutputCalculator(
		QSharedPointer<iASingleResult> result,
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
		connected->SetDistanceThreshold(0.5);
		if (m_result->GetLabelledImage().IsNull())
		{
			DEBUG_LOG("Labelled Image is null");
			m_success = false;
			return;
		}
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(m_result->GetLabelledImage().GetPointer());
		connected->SetInput(lblImg);
		connected->Update();
		m_result->DiscardDetails();
		typedef itk::RelabelComponentImageFilter <OutputImageType, OutputImageType >
			RelabelFilterType;
		RelabelFilterType::Pointer relabel = RelabelFilterType::New();
		relabel->SetInput(connected->GetOutput());
		//relabel->SetSortByObjectSize(false);
		relabel->Update();
		int objCount = relabel->GetNumberOfObjects();
		m_result->SetAttribute(m_objCountIdx, objCount);

		if (m_result->ProbabilityAvailable())
		{
			typedef itk::ImageRegionConstIterator<ProbabilityImageType> ConstDblIt;
			typedef fhw::EntropyImageFilter<ProbabilityImageType, ProbabilityImageType> EntropyFilter;
			auto entropyFilter = EntropyFilter::New();
			for (int i = 0; i < m_labelCount; ++i)
			{
				ProbabilityImageType* probImg = dynamic_cast<ProbabilityImageType*>(m_result->GetProbabilityImg(i).GetPointer());
				entropyFilter->SetInput(i, probImg);
			}
			entropyFilter->SetNormalize(true);
			entropyFilter->Update();
			double avgEntropy;
			getStatistics(entropyFilter->GetOutput(), nullptr, nullptr, &avgEntropy);
			m_result->SetAttribute(m_avgUncIdx, avgEntropy);
			m_result->DiscardProbability();
		}
	} catch (std::exception & e)
	{
		DEBUG_LOG(QString("An exception occured while computing derived output: %1").arg(e.what()));
		m_success = false;
	}
	/*
	itk::ImageFileWriter<OutputImageType>::Pointer writer = itk::ImageFileWriter<OutputImageType>::New();
	writer->SetFileName(debugCount.toStdString() );
	writer->SetUseCompression(true);
	writer->SetInput(relabel->GetOutput() );
	writer->Update();
	*/
}


bool iADerivedOutputCalculator::success()
{
	return m_success;
}