// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASingleResult.h"

#include "iAAttributes.h"
#include "iASamplingResults.h"

#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iANameMapper.h>
#include <iAToolsITK.h>
#include <iAFileUtils.h>
#include <iAITKIO.h>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

const QString iASingleResult::ValueSplitString(",");

std::shared_ptr<iASingleResult> iASingleResult::create(
	QString const & line,
	iASamplingResults const & sampling,
	std::shared_ptr<iAAttributes> attributes,
	bool showErrorOutput)
{
	QStringList tokens = line.split(ValueSplitString);

	bool ok = false;
	int id = tokens[0].toInt(&ok);
	if (!ok)
	{
		// legacy format: split string " ":
		QRegularExpression sep("(,| )");
		tokens = line.split(sep);
		id = tokens[0].toInt(&ok);
		if (!ok)
		{
			if (showErrorOutput)
			{
				LOG(lvlError, QString("Invalid result ID: %1").arg(tokens[0]));
			}
			return std::shared_ptr<iASingleResult>();
		}
		else
		{
			LOG(lvlWarn, "Legacy format .sps/.chr files detected, consider replacing ' ' by ',' in those files!");
		}
	}
	std::shared_ptr<iASingleResult> result(new iASingleResult(
		id,
		sampling
	));
	if (tokens.size() < attributes->size()+1) // +1 for ID
	{
		if (showErrorOutput)
		{
			LOG(lvlError, QString("Invalid token count(=%1), expected %2").arg(tokens.size()).arg(attributes->size() + 1));
		}
		return std::shared_ptr<iASingleResult>();
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
			if (showErrorOutput)
			{
				LOG(lvlError, QString("Could not parse attribute value # %1: '%2' (type=%3).")
					.arg(i).arg(curToken).arg(ValueType2Str(valueType)));
			}
			return std::shared_ptr<iASingleResult>();
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

std::shared_ptr<iASingleResult> iASingleResult::create(
	int id,
	iASamplingResults const & sampling,
	QVector<QVariant> const & parameter,
	QString const & fileName)
{
	std::shared_ptr<iASingleResult> result(new iASingleResult(id, sampling));
	result->m_attributeValues = parameter;
	result->m_fileName = fileName;
	return result;
}

QString iASingleResult::toString(std::shared_ptr<iAAttributes> attributes, int type)
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
	if (type == iAAttributeDescriptor::Parameter)
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
	QFileInfo f(labelPath());
	if (!f.exists() || f.isDir())
	{
		LOG(lvlError, QString("Label Image %1 does not exist, or is not a file!").arg(labelPath()));
		return false;
	}
	iAITKIO::PixelType pixelType;
	iAITKIO::ScalarType scalarType;
	m_labelImg = iAITKIO::readFile(labelPath(), pixelType, scalarType, false);
	assert(pixelType == iAITKIO::PixelType::SCALAR);
	if (scalarType != iAITKIO::ScalarType::INT)
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
		iAITKIO::PixelType pixelType;
		iAITKIO::ScalarType scalarType;
		m_probabilityImg[label] = iAITKIO::readFile(probFile, pixelType, scalarType, false);
		assert(pixelType == iAITKIO::PixelType::SCALAR);
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
		iAITKIO::PixelType pixelType;
		iAITKIO::ScalarType scalarType;
		probabilityImg[l] = dynamic_cast<ProbabilityImageType*>(iAITKIO::readFile(probFile, pixelType, scalarType, false).GetPointer());
		assert(pixelType == iAITKIO::PixelType::SCALAR);
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

std::shared_ptr<iAAttributes> iASingleResult::attributes() const
{
	return m_sampling.attributes();
}
