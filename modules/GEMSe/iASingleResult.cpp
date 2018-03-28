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

#include "iASingleResult.h"

#include "iAAttributeDescriptor.h"
#include "iAAttributes.h"
#include "iANameMapper.h"
#include "iASamplingResults.h"

#include "iAConsole.h"
#include "io/iAFileUtils.h"
#include "io/iAITKIO.h"
#include "iAToolsITK.h"

#include <QFile>
#include <QFileInfo>

const QString iASingleResult::ValueSplitString(",");

QSharedPointer<iASingleResult> iASingleResult::Create(
	QString const & line,
	iASamplingResults const & sampling,
	QSharedPointer<iAAttributes> attributes)
{
	QStringList tokens = line.split(ValueSplitString);

	bool ok = false;
	int id = tokens[0].toInt(&ok);
	if (!ok)
	{
		DEBUG_LOG(QString("Invalid result ID: %1").arg(id));
		return QSharedPointer<iASingleResult>();
	}
	QSharedPointer<iASingleResult> result(new iASingleResult(
		id,
		sampling
	));
	if (tokens.size() < attributes->size()+1) // +1 for ID
	{
		DEBUG_LOG(QString("Invalid token count(=%1), expected %2").arg(tokens.size()).arg(attributes->size()+1));
		return QSharedPointer<iASingleResult>();
	}
	for (int i = 0; i < attributes->size(); ++i)
	{
		double value = -1;
		int valueType = attributes->at(i)->ValueType();
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
				value = attributes->at(i)->NameMapper()->GetIdx(curToken, ok);
				break;
		}
		if (!ok)
		{
			DEBUG_LOG(QString("Could not parse attribute value # %1: '%2' (type=%3).").arg(i).arg(curToken).arg((valueType==Continuous?"Continuous": valueType == Discrete? "Discrete":"Categorical")));
			return QSharedPointer<iASingleResult>();
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

QSharedPointer<iASingleResult> iASingleResult::Create(
	int id,
	iASamplingResults const & sampling,
	QVector<double> const & parameter,
	QString const & fileName)
{
	QSharedPointer<iASingleResult> result(new iASingleResult(id, sampling));
	result->m_attributeValues = parameter;
	result->m_fileName = fileName;
	return result;
}


QString iASingleResult::ToString(QSharedPointer<iAAttributes> attributes, int type)
{
	QString result;
	if (attributes->size() != m_attributeValues.size())
	{
		DEBUG_LOG("Non-matching attribute list given (number of descriptors and number of values don't match).");
		return result;
	}
	for (int i = 0; i < m_attributeValues.size(); ++i)
	{
		if (attributes->at(i)->AttribType() == type)
		{
			if (!result.isEmpty())
			{
				result += ValueSplitString;
			}
			if (attributes->at(i)->NameMapper())
			{
				result += attributes->at(i)->NameMapper()->GetName(m_attributeValues[i]);
			}
			else
			{
				result += (attributes->at(i)->ValueType() == iAValueType::Discrete) ?
					QString::number(static_cast<int>(m_attributeValues[i])) :
					QString::number(m_attributeValues[i]);
			}
		}
	}
	if (type == iAAttributeDescriptor::DerivedOutput)
	{
		result += ValueSplitString + MakeRelative(m_sampling.GetPath(), m_fileName);
	}
	return result;
}


iASingleResult::iASingleResult(int id, iASamplingResults const & sampling):
	m_id(id),
	m_sampling(sampling)
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
	QFileInfo f(GetLabelPath());
	if (!f.exists() || f.isDir())
	{
		DEBUG_LOG(QString("Label Image %1 does not exist, or is not a file!").arg(GetLabelPath()));
		return false;
	}
	m_labelImg = iAITKIO::readFile(GetLabelPath(), pixelType, false);
	if (pixelType != itk::ImageIOBase::INT)
	{
		m_labelImg = CastImageTo<int>(m_labelImg);
	}
	return (m_labelImg);
}

void iASingleResult::DiscardDetails()
{
	m_labelImg = nullptr;
}

void iASingleResult::DiscardProbability()
{
	for (int i = 0; i < m_probabilityImg.size(); ++i)
	{
		m_probabilityImg[i] = nullptr;
	}
}

double iASingleResult::GetAttribute(int id) const
{
	return m_attributeValues[id];
}

void iASingleResult::SetAttribute(int id, double value)
{
	if (id >= m_attributeValues.size())
	{
		// DEBUG_LOG(QString("Set attribute idx (=%1) > current size (=%2)\n").arg(id).arg(m_attributeValues.size()));
		m_attributeValues.resize(id + 1);
	}
	m_attributeValues[id] = value;
}

iAITKIO::ImagePointer iASingleResult::GetProbabilityImg(int label)
{
	if (m_probabilityImg.size() <= label)
	{
		m_probabilityImg.resize(label +1);
	}
	if (!m_probabilityImg[label])
	{
		QString probFile(GetProbabilityPath(label));
		if (!QFile::exists(probFile))
		{
			throw std::runtime_error(QString("File %1 does not exist!").arg(probFile).toStdString().c_str());
		}
		iAITKIO::ScalarPixelType pixelType;
		m_probabilityImg[label] = iAITKIO::readFile(probFile, pixelType, false);
	}
	return m_probabilityImg[label];
}

bool iASingleResult::ProbabilityAvailable() const
{
	if (m_probabilityImg.size() > 0)
		return true;

	QString probFile(GetProbabilityPath(0));
	return QFile::exists(probFile);
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
	return m_sampling.GetPath(m_id);
}


QString iASingleResult::GetLabelPath() const
{
	return m_fileName;
}

QString iASingleResult::GetProbabilityPath(int label) const
{
	return QString("%1/prob%2.mhd").arg(GetFolder()).arg(label);
}


int iASingleResult::GetDatasetID() const
{
	return m_sampling.GetID();
}

QSharedPointer<iAAttributes> iASingleResult::GetAttributes() const
{
	return m_sampling.GetAttributes();
}
