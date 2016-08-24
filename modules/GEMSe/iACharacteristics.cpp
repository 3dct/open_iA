/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iACharacteristics.h"

#include "iAConsole.h"
#include "iASingleResult.h"
#include "iAAttributes.h"

#include <itkImageFileWriter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>

#include <QString>

// TODO: Remove!
const int DIM = 3;
typedef int LabelPixelType;
typedef itk::Image<LabelPixelType, DIM> LabelImageType;
typedef LabelImageType::Pointer LabelImagePointer;

CharacteristicsCalculator::CharacteristicsCalculator(
		QSharedPointer<iASingleResult> result,
		QSharedPointer<iAAttributes> range,
		int objCountIdx):
	m_result(result),
	m_range(range),
	m_objCountIdx(objCountIdx),
	m_success(true)
{}


void CharacteristicsCalculator::run()
{
	try
	{
		typedef itk::Image< unsigned int, 3 > OutputImageType;
		typedef itk::ScalarConnectedComponentImageFilter <LabelImageType, OutputImageType > ConnectedComponentImageFilterType;
		ConnectedComponentImageFilterType::Pointer connected = ConnectedComponentImageFilterType::New();
		connected->SetDistanceThreshold(0.5);
		if (m_result->GetLabelledImage().IsNull())
		{
			DEBUG_LOG("Labelled Image is null\n");
			m_success = false;
			return;
		}
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(m_result->GetLabelledImage().GetPointer());
		connected->SetInput(lblImg);
		connected->Update();
		typedef itk::RelabelComponentImageFilter <OutputImageType, OutputImageType >
			RelabelFilterType;
		RelabelFilterType::Pointer relabel = RelabelFilterType::New();
		relabel->SetInput(connected->GetOutput());
		//relabel->SetSortByObjectSize(false);
		relabel->Update();
		int objCount = relabel->GetNumberOfObjects();
		m_result->SetAttribute(m_objCountIdx, objCount);
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


bool CharacteristicsCalculator::success()
{
	return m_success;
}