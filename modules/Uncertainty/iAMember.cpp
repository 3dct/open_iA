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

#include "iAMember.h"

#include "iAAttributeDescriptor.h"
#include "iAAttributes.h"
#include "iANameMapper.h"
#include "iASamplingResults.h"

#include "iAConsole.h"
#include "iAFileUtils.h"
#include "iAToolsITK.h"
#include "iAITKIO.h"

#include <QFile>
#include <QFileInfo>

QSharedPointer<iAMember> iAMember::create(
	QString const & line,
	iASamplingResults const & sampling,
	QSharedPointer<iAAttributes> attributes)
{
	QStringList tokens = line.split(iAAttributeDescriptor::ValueSplitString);

	bool ok = false;
	int id = tokens[0].toInt(&ok);
	if (!ok)
	{
		DEBUG_LOG(QString("Invalid result ID: %1").arg(id));
		return QSharedPointer<iAMember>();
	}
	QSharedPointer<iAMember> result(new iAMember(
		id,
		sampling
	));
	if (tokens.size() < attributes->size()+1) // +1 for ID
	{
		DEBUG_LOG(QString("Invalid token count(=%1), expected %2").arg(tokens.size()).arg(attributes->size()+1));
		return QSharedPointer<iAMember>();
	}
	for (int i = 0; i < attributes->size(); ++i)
	{
		double value = -1;
		int valueType = attributes->at(i)->GetValueType();
		QString curToken = tokens[i + 1];
		switch (valueType)
		{
			case Continuous:
				value = curToken.toDouble(&ok);
				break;
			case Discrete:
				value = curToken.toInt(&ok);
				break;
			case Categorical:
				value = attributes->at(i)->GetNameMapper()->GetIdx(curToken, ok);
				break;
		}
		if (!ok)
		{
			DEBUG_LOG(QString("Could not parse attribute value # %1: '%2' (type=%3).").arg(i).arg(curToken).arg((valueType==Continuous?"Continuous": valueType == Discrete? "Discrete":"Categorical")));
			return QSharedPointer<iAMember>();
		}
		result->m_attributeValues.push_back(value);
	}
	if (tokens.size() > attributes->size() + 1) // fileName at end
	{
		result->m_fileName = MakeAbsolute(sampling.GetPath(), tokens[attributes->size() + 1]);
	}
	else
	{
		result->m_fileName = result->GetFolder() + "/label.mhd";
	}
	return result;
}

QSharedPointer<iAMember> iAMember::create(
	int id,
	iASamplingResults const & sampling,
	QVector<double> const & parameter,
	QString const & fileName)
{
	QSharedPointer<iAMember> result(new iAMember(id, sampling));
	result->m_attributeValues = parameter;
	result->m_fileName = fileName;
	return result;
}


QString iAMember::ToString(QSharedPointer<iAAttributes> attributes, int type)
{
	QString result;
	if (attributes->size() != m_attributeValues.size())
	{
		DEBUG_LOG("Non-matching attribute list given (number of descriptors and number of values don't match).");
		return result;
	}
	for (int i = 0; i < m_attributeValues.size(); ++i)
	{
		if (attributes->at(i)->GetAttribType() == type)
		{
			if (!result.isEmpty())
			{
				result += iAAttributeDescriptor::ValueSplitString;
			}
			if (attributes->at(i)->GetNameMapper())
			{
				result += attributes->at(i)->GetNameMapper()->GetName(m_attributeValues[i]);
			}
			else
			{
				result += (attributes->at(i)->GetValueType() == iAValueType::Discrete) ?
					QString::number(static_cast<int>(m_attributeValues[i])) :
					QString::number(m_attributeValues[i]);
			}
		}
	}
	if (type == iAAttributeDescriptor::DerivedOutput)
	{
		result += iAAttributeDescriptor::ValueSplitString + MakeRelative(m_sampling.GetPath(), m_fileName);
	}
	return result;
}


iAMember::iAMember(int id, iASamplingResults const & sampling):
	m_id(id),
	m_sampling(sampling)
{
}

int iAMember::GetID()
{
	return m_id;
}

iAITKIO::ImagePointer const iAMember::LabelImage()
{
	iAITKIO::ScalarPixelType pixelType;
	QFileInfo f(GetLabelPath());
	if (!f.exists() || f.isDir())
	{
		DEBUG_LOG(QString("Label Image %1 does not exist, or is not a file!").arg(GetLabelPath()));
		return false;
	}
	iAITKIO::ImagePointer labelImg = iAITKIO::readFile(GetLabelPath(), pixelType, false);
	if (pixelType != itk::ImageIOBase::INT)
	{
		labelImg = CastImageTo<int>(labelImg);
	}
	return labelImg;
}

double iAMember::GetAttribute(int id) const
{
	return m_attributeValues[id];
}

void iAMember::SetAttribute(int id, double value)
{
	if (id >= m_attributeValues.size())
	{
		// DEBUG_LOG(QString("Set attribute idx (=%1) > current size (=%2)\n").arg(id).arg(m_attributeValues.size()));
		m_attributeValues.resize(id + 1);
	}
	m_attributeValues[id] = value;
}

QVector<DoubleImage::Pointer> iAMember::GetProbabilityImgs(int labelCount)
{
	QVector<DoubleImage::Pointer> probabilityImg(labelCount);
	for (int l=0; l<labelCount; ++l)
	{
		QString probFile(GetProbabilityPath(l));
		if (!QFile::exists(probFile))
		{
			throw std::runtime_error(QString("File %1 does not exist!").arg(probFile).toStdString().c_str());
		}
		iAITKIO::ScalarPixelType pixelType;
		probabilityImg[l] = dynamic_cast<DoubleImage*>(iAITKIO::readFile(probFile, pixelType, false).GetPointer());
	}
	return probabilityImg;
}

bool iAMember::ProbabilityAvailable() const
{
	QString probFile(GetProbabilityPath(0));
	return QFile::exists(probFile);
}


QString iAMember::GetFolder() const
{
	return m_sampling.GetPath(m_id);
}


QString iAMember::GetLabelPath() const
{
	return m_fileName;
}

QString iAMember::GetProbabilityPath(int label) const
{
	return QString("%1/prob%2.mhd").arg(GetFolder()).arg(label);
}


int iAMember::GetDatasetID() const
{
	return m_sampling.GetID();
}

QSharedPointer<iAAttributes> iAMember::GetAttributes() const
{
	return m_sampling.GetAttributes();
}
