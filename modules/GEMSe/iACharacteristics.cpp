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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iACharacteristics.h"

#include "iAConsole.h"
#include "iAImageTypes.h"
#include "iASingleResult.h"
#include "iAAttributes.h"

#include <itkImageFileWriter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>

#include <QString>

iACharacteristics::iACharacteristics():
m_objectCount(0),
m_duration(0.0)
{
	std::fill(m_measures, m_measures+MeasureCount, 0.0);
}

int iACharacteristics::ObjectCount() const
{
	return m_objectCount;
}

void iACharacteristics::SetObjectCount(int objCount)
{
	m_objectCount = objCount;
}

double iACharacteristics::Duration() const
{
	return m_duration;
}

void iACharacteristics::SetDuration(double value)
{
	m_duration = value;
}

void iACharacteristics::SetMeasures(double value[MeasureCount])
{
	for (int i=0; i<MeasureCount; ++i)
	{
		m_measures[i] = value[i];
	}
}

double iACharacteristics::Measure(int idx) const
{
	return m_measures[idx];
}

iACharacteristics iACharacteristics::Create(QString const & descriptor)
{
	iACharacteristics result;
	QStringList tokens = descriptor.split(" ");
	if (tokens.size() == 0)
	{
		DEBUG_LOG(QString("Characteristics: Invalid line '%1'\n").arg(descriptor));
		return result;
	}
	bool ok;
	result.m_objectCount = tokens[0].toInt(&ok);
	if (!ok)
	{
		DEBUG_LOG(QString("Characteristics: Invalid objectCount in line '%1'\n").arg(descriptor));
	}
	ok = true;
	result.m_duration = (tokens.size() > 1) ? tokens[1].toDouble(&ok) : 0;
	if (!ok)
	{
		DEBUG_LOG(QString("Characteristics:Invalid duration in line '%1'\n").arg(descriptor));
	}
	if (tokens.size() <= 1)
	{
		DEBUG_LOG(QString("Characteristics: Missing duration in line '%1'\n").arg(descriptor));
		return result;
	}
	return result;
}

QString iACharacteristics::GetDescriptor() const
{
	return QString::number(m_objectCount) + " " + QString::number(m_duration);
}

CharacteristicsCalculator::CharacteristicsCalculator(QSharedPointer<iASingleResult> result, QSharedPointer<iAAttributes> range, int objCountIdx):
m_result(result), m_range(range),
m_objCountIdx(objCountIdx)
{}


void CharacteristicsCalculator::run()
{
	typedef itk::Image< unsigned int, 3 > OutputImageType;
	typedef itk::ScalarConnectedComponentImageFilter <LabelImageType, OutputImageType > ConnectedComponentImageFilterType;
	ConnectedComponentImageFilterType::Pointer connected = ConnectedComponentImageFilterType::New ();
	connected->SetDistanceThreshold(0.5);
	if (m_result->GetLabelledImage().IsNull())
	{
		DEBUG_LOG("Labelled Image is null\n");
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
	/*
	itk::ImageFileWriter<OutputImageType>::Pointer writer = itk::ImageFileWriter<OutputImageType>::New();
	writer->SetFileName(debugCount.toStdString() );
	writer->SetUseCompression(true);
	writer->SetInput(relabel->GetOutput() );
	writer->Update();
	*/
}