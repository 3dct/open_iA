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

#include "iASingleResult.h"

#include "iAAttributeDescriptor.h"
#include "iAAttributes.h"
#include "iAGEMSeConstants.h"
#include "iANameMapper.h"

#include "iAConsole.h"
#include "iAToolsITK.h"
#include "iAITKIO.h"

QSharedPointer<iASingleResult> iASingleResult::Create(
	QString const & line, QString const & path, QSharedPointer<iAAttributes> attributes)
{
	QStringList tokens = line.split(ValueSplitString);

	bool ok = false;
	int id = tokens[0].toInt(&ok);
	if (!ok)
	{
		DEBUG_LOG(QString("Invalid result ID: %1").arg(id));
		return QSharedPointer<iASingleResult>();
	}
	QSharedPointer<iASingleResult> result(new iASingleResult(id, path + "/sample" + QString::number(id)));
	if (tokens.size() != attributes->size()+1) // +1 for ID
	{
		DEBUG_LOG(QString("Invalid token count(=%1), expected %2").arg(tokens.size()).arg(attributes->size()+1));
		return QSharedPointer<iASingleResult>();
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
			DEBUG_LOG(QString("Could not parse attribute value # %1: '%2' (type=%3)\n").arg(i).arg(curToken).arg((valueType==Continuous?"Continuous": valueType == Discrete? "Discrete":"Categorical")));
			return QSharedPointer<iASingleResult>();
		}
		result->m_attributeValues.push_back(value);
	}
	return result;
}

QSharedPointer<iASingleResult> iASingleResult::Create(int id, QString const & path,
	QVector<double> const & parameter)
{
	QSharedPointer<iASingleResult> result(new iASingleResult(id, path + "/sample" + QString::number(id)));
	result->m_attributeValues = parameter;
	return result;
}


QString iASingleResult::ToString(QSharedPointer<iAAttributes> attributes, int type)
{
	QString result;
	if (attributes->size() != m_attributeValues.size())
	{
		DEBUG_LOG("Non-matching attribute list given (number of descriptors and number of values don't match\n");
		return result;
	}
	for (int i = 0; i < m_attributeValues.size(); ++i)
	{
		if (attributes->at(i)->GetAttribType() == type)
		{
			if (!result.isEmpty())
			{
				result += ValueSplitString;
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
	return result;
}


iASingleResult::iASingleResult(int id, QString const & path):
	m_id(id),
	m_path(path)
{
}

int iASingleResult::GetID()
{
	return m_id;
}

iAITKIO::ImagePointer const iASingleResult::GetLabelledImage()
{
	if (!m_labelImg)
	{
		LoadLabelImage();
	}
	return m_labelImg;
}

bool iASingleResult::LoadLabelImage()
{
	iAITKIO::ScalarPixelType pixelType;
	m_labelImg = iAITKIO::readFile(QString(m_path + "/label.mhd"), pixelType);
	return (m_labelImg);
}

void iASingleResult::DiscardDetails()
{
	m_labelImg = NULL;
}

//! get attribute (parameter or characteristic)
double iASingleResult::GetAttribute(int id) const
{
	return m_attributeValues[id];
}

//! set attribute (parameter or characteristic)
void iASingleResult::SetAttribute(int id, double value)
{
	if (id >= m_attributeValues.size())
	{
		// DEBUG_LOG(QString("Set attribute idx (=%1) > current size (=%2)\n").arg(id).arg(m_attributeValues.size()));
		m_attributeValues.resize(id + 1);
	}
	m_attributeValues[id] = value;
}

iAITKIO::ImagePointer iASingleResult::GetProbabilityImg(int l)
{
	if (m_probabilityImg.size() <= l)
	{
		m_probabilityImg.resize(l+1);
	}
	if (!m_probabilityImg[l])
	{
		iAITKIO::ScalarPixelType pixelType;
		m_probabilityImg[l] = iAITKIO::readFile(QString("%1/prob%2.mhd").arg(m_path).arg(l), pixelType);
	}
	return m_probabilityImg[l];
}


void iASingleResult::SetLabelImage(iAITKIO::ImagePointer labelImg)
{
	m_labelImg = labelImg;
}


void iASingleResult::AddProbabilityImages(QVector<iAITKIO::ImagePointer> & probImgs)
{
	m_probabilityImg = probImgs;
}


QString iASingleResult::GetFolder() const
{
	return m_path;
}