/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAAttributes.h"
#include "iASamplingResults.h"

#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iANameMapper.h>
#include <iAToolsITK.h>
#include <io/iAFileUtils.h>
#include <io/iAITKIO.h>

#include <QFile>
#include <QFileInfo>

const QString iASingleResult::ValueSplitString(",");

QSharedPointer<iASingleResult> iASingleResult::create(
	QString const & line,
	iASamplingResults const & sampling,
	QSharedPointer<iAAttributes> attributes)
{
	QStringList tokens = line.split(ValueSplitString);

	bool ok = false;
	int id = tokens[0].toInt(&ok);
	if (!ok)
	{
		// legacy format: split string " ":
		QRegExp sep("(,| )");
		tokens = line.split(sep);
		id = tokens[0].toInt(&ok);
		if (!ok)
		{
			LOG(lvlError, QString("Invalid result ID: %1").arg(tokens[0]));
			return QSharedPointer<iASingleResult>();
		}
		else
		{
			LOG(lvlWarn, "Legacy format .sps/.chr files detected, consider replacing ' ' by ',' in those files!");
		}
	}
	QSharedPointer<iASingleResult> result(new iASingleResult(
		id,
		sampling
	));
	if (tokens.size() < attributes->size()+1) // +1 for ID
	{
		LOG(lvlError, QString("Invalid token count(=%1), expected %2").arg(tokens.size()).arg(attributes->size()+1));
		return QSharedPointer<iASingleResult>();
	}
	for (int i = 0; i < attributes->size(); ++i)
	{
		QVariant value;
		auto valueType = attributes->at(i)->valueType();
		QString curToken = tokens[i + 1];
		switch (valueType)
		{
			case iAValueType::Continuous:
				value = curToken.toDouble(&ok);
				break;
			case iAValueType::Discrete:
				value = curToken.toInt(&ok);
				break;
			case iAValueType::Categorical:
				value = attributes->at(i)->nameMapper()->GetIdx(curToken, ok);
				break;
			default:
				value = curToken;
				break;
		}
		if (!ok)
		{
			LOG(lvlError, QString("Could not parse attribute value # %1: '%2' (type=%3).")
				.arg(i).arg(curToken).arg(ValueType2Str(valueType)));
			return QSharedPointer<iASingleResult>();
		}
		result->m_attributeValues.push_back(value);
	}
	if (tokens.size() > attributes->size() + 1) // fileName at end
	{
		result->m_fileName = MakeAbsolute(sampling.path(), tokens[attributes->size() + 1]);
	}
	else
	{
		result->m_fileName = result->folder() + "/label.mhd";
	}
	return result;
}

QSharedPointer<iASingleResult> iASingleResult::create(
	int id,
	iASamplingResults const & sampling,
	QVector<QVariant> const & parameter,
	QString const & fileName)
{
	QSharedPointer<iASingleResult> result(new iASingleResult(id, sampling));
	result->m_attributeValues = parameter;
	result->m_fileName = fileName;
	return result;
}

QString iASingleResult::toString(QSharedPointer<iAAttributes> attributes, int type)
{
	QString result;
	if (attributes->size() != m_attributeValues.size())
	{
		LOG(lvlError, "Non-matching attribute list given (number of descriptors and number of values don't match).");
		return result;
	}
	for (int i = 0; i < m_attributeValues.size(); ++i)
	{
		if (attributes->at(i)->attribType() == type)
		{
			if (!result.isEmpty())
			{
				result += ValueSplitString;
			}
			if (attributes->at(i)->valueType() == iAValueType::Categorical &&
				m_attributeValues[i].toInt() < attributes->at(i)->nameMapper()->size())
			{
				result += attributes->at(i)->nameMapper()->name(m_attributeValues[i].toInt());
			}
			else
			{
				result += (attributes->at(i)->valueType() == iAValueType::Discrete) ?
					QString::number(m_attributeValues[i].toInt()) :
					m_attributeValues[i].toString();
			}
		}
	}
	if (type == iAAttributeDescriptor::DerivedOutput)
	{
		result += ValueSplitString + MakeRelative(m_sampling.path(), m_fileName);
	}
	return result;
}

iASingleResult::iASingleResult(int id, iASamplingResults const & sampling):
	m_sampling(sampling),
	m_id(id)
{
}

iAITKIO::ImagePointer const iASingleResult::labelImage()
{
	if (!m_labelImg)
	{
		loadLabelImage();
	}
	return m_labelImg;
}

bool iASingleResult::loadLabelImage()
{
	iAITKIO::ScalarPixelType pixelType;
	QFileInfo f(labelPath());
	if (!f.exists() || f.isDir())
	{
		LOG(lvlError, QString("Label Image %1 does not exist, or is not a file!").arg(labelPath()));
		return false;
	}
	m_labelImg = iAITKIO::readFile(labelPath(), pixelType, false);
	if (pixelType != itk::ImageIOBase::INT)
	{
		m_labelImg = castImageTo<int>(m_labelImg);
	}
	return (m_labelImg);
}

void iASingleResult::discardDetails()
{
	m_labelImg = nullptr;
}

void iASingleResult::discardProbability()
{
	for (int i = 0; i < m_probabilityImg.size(); ++i)
	{
		m_probabilityImg[i] = nullptr;
	}
}

double iASingleResult::attribute(int id) const
{
	return m_attributeValues[id].toDouble();
}

void iASingleResult::setAttribute(int id, double value)
{
	if (id >= m_attributeValues.size())
	{
		// LOG(lvlWarn, QString("Set attribute idx (=%1) > current size (=%2)\n").arg(id).arg(m_attributeValues.size()));
		m_attributeValues.resize(id + 1);
	}
	m_attributeValues[id] = value;
}

iAITKIO::ImagePointer iASingleResult::probabilityImg(int label)
{
	if (m_probabilityImg.size() <= label)
	{
		m_probabilityImg.resize(label +1);
	}
	if (!m_probabilityImg[label])
	{
		QString probFile(probabilityPath(label));
		if (!QFile::exists(probFile))
		{
			throw std::runtime_error(QString("File %1 does not exist!").arg(probFile).toStdString().c_str());
		}
		iAITKIO::ScalarPixelType pixelType;
		m_probabilityImg[label] = iAITKIO::readFile(probFile, pixelType, false);
	}
	return m_probabilityImg[label];
}

QVector<ProbabilityImagePointer> iASingleResult::probabilityImgs(int labelCount)
{
	QVector<ProbabilityImagePointer> probabilityImg(labelCount);
	for (int l = 0; l < labelCount; ++l)
	{
		QString probFile(probabilityPath(l));
		if (!QFile::exists(probFile))
		{
			throw std::runtime_error(QString("File %1 does not exist!").arg(probFile).toStdString().c_str());
		}
		iAITKIO::ScalarPixelType pixelType;
		probabilityImg[l] = dynamic_cast<ProbabilityImageType*>(iAITKIO::readFile(probFile, pixelType, false).GetPointer());
	}
	return probabilityImg;
}

bool iASingleResult::probabilityAvailable() const
{
	if (m_probabilityImg.size() > 0)
		return true;

	QString probFile(probabilityPath(0));
	return QFile::exists(probFile);
}

void iASingleResult::setLabelImage(iAITKIO::ImagePointer labelImg)
{
	m_labelImg = labelImg;
}

void iASingleResult::addProbabilityImages(QVector<iAITKIO::ImagePointer> & probImgs)
{
	m_probabilityImg = probImgs;
}

QString iASingleResult::folder() const
{
	return m_sampling.path(m_id);
}

QString iASingleResult::labelPath() const
{
	return m_fileName;
}

QString iASingleResult::probabilityPath(int label) const
{
	return QString("%1/prob%2.mhd").arg(folder()).arg(label);
}

int iASingleResult::id() const
{
	return m_id;
}

int iASingleResult::datasetID() const
{
	return m_sampling.id();
}

QSharedPointer<iAAttributes> iASingleResult::attributes() const
{
	return m_sampling.attributes();
}
