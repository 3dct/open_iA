/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAAttributeDescriptor.h"

#include "iAConsole.h"
#include "iAListNameMapper.h"
#include "iAStringHelper.h"

const QString iAAttributeDescriptor::ValueSplitString(",");

namespace
{
	const QString AttributeSplitString("\t");
	const QString CategoricalValueSplitString(",");

	// Attribute Types:
	const QString ParameterStr("Parameter");
	const QString DerivedOutputStr("Derived Output");

	const QString LinearStr("Linear");
	const QString LogarithmicStr("Logarithmic");

	const QString UnknownStr("Unknown");
}


iAAttributeDescriptor::iAAttributeType Str2AttribType(QString const & str)
{
	if (str == ParameterStr)
	{
		return iAAttributeDescriptor::Parameter;
	}
	else if (str == DerivedOutputStr)
	{
		return iAAttributeDescriptor::DerivedOutput;
	}
	else
	{
		DEBUG_LOG(QString("Unknown attribute descriptor '%1'\n").arg(str));
		return iAAttributeDescriptor::None;
	}
}

QString AttribType2Str(iAAttributeDescriptor::iAAttributeType type)
{
	switch (type)
	{
		case iAAttributeDescriptor::Parameter:
			return ParameterStr;
		case iAAttributeDescriptor::DerivedOutput:
			return DerivedOutputStr;
		default:
			return UnknownStr;
	}
}

QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::create(QString const & def)
{
	QStringList defTokens = def.split(AttributeSplitString);
	if (defTokens.size() < 3)
	{
		DEBUG_LOG(QString("Not enough tokens in attribute descriptor '%1'").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	QSharedPointer<iAAttributeDescriptor> result(new iAAttributeDescriptor(
			defTokens[0], Str2AttribType(defTokens[1]), Str2ValueType(defTokens[2])
	));
	int requiredTokens = (result->valueType() == Boolean) ? 3 :
		((result->valueType() == Categorical) ? 4 : 5);
	if (defTokens.size() < requiredTokens)
	{
		DEBUG_LOG(QString("Not enough tokens in attribute descriptor '%1'").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	if (result->valueType() == Continuous || result->valueType() == Discrete)
	{
		result->m_logarithmic = false;
		bool minOk = true, maxOk = true;
		result->m_min = iAConverter<double>::toT(defTokens[3], &minOk);
		result->m_max = iAConverter<double>::toT(defTokens[4], &maxOk);
		if (!minOk || !maxOk)
		{
			DEBUG_LOG(QString("Minimum or maximum of attribute couldn't be parsed in line %1\n").arg(def));
			return QSharedPointer<iAAttributeDescriptor>();
		}
		if (defTokens.size() >= 6)
		{
			result->m_logarithmic = (defTokens[5] == LogarithmicStr);
		}
		if (defTokens.size() > 6)
		{
			DEBUG_LOG(QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
		}
	}
	else if (result->valueType() == Categorical)
	{
		QStringList categories = defTokens[3].split(CategoricalValueSplitString);
		result->m_min = 0;
		result->m_max = categories.size()-1;
		result->m_nameMapper = QSharedPointer<iAListNameMapper>(new iAListNameMapper(categories));
		if (defTokens.size() > 5)
		{
			DEBUG_LOG(QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
		}
	}
	return result;
}

QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::createParam(
	QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	auto result = QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(name, Parameter, valueType));
	result->m_min = min;
	result->m_max = max;
	result->m_defaultValue = defaultValue;
	return result;
}

QString iAAttributeDescriptor::toString() const
{
	QString result = name() + AttributeSplitString +
		AttribType2Str(attribType()) + AttributeSplitString +
		ValueType2Str(valueType()) + AttributeSplitString;
	if (valueType() == Continuous || valueType() == Discrete)
	{
		result += QString::number(min()) + AttributeSplitString + QString::number(max()) + AttributeSplitString + (m_logarithmic ? LogarithmicStr : LinearStr);
	}
	else if (valueType() == iAValueType::Categorical)
	{
		if (!m_nameMapper)
		{
			DEBUG_LOG("nameMapper nullptr for categorical attribute!\n");
			for (int i = min(); i <= max(); ++i)
			{
				result += QString::number(i);
				if (i < max())
				{
					result += CategoricalValueSplitString;
				}
			}
		}
		else
		{
			for (int i = min(); i <= max(); ++i)
			{
				result += m_nameMapper->name(i);
				if (i < m_nameMapper->size() - 1)
				{
					result += CategoricalValueSplitString;
				}
			}
		}
	}
	return result + "\n";
}

iAAttributeDescriptor::iAAttributeDescriptor(
		QString const & name, iAAttributeType attribType, iAValueType valueType) :
	m_attribType(attribType),
	m_valueType(valueType),
	m_logarithmic(false),
	m_name(name)
{
	resetMinMax();	// TODO: check why we set it in constructor when it's reset again here anyway; maybe move this to where it's actually needed?
}

iAAttributeDescriptor::~iAAttributeDescriptor()
{}

iAAttributeDescriptor::iAAttributeType iAAttributeDescriptor::attribType() const
{
	return m_attribType;
}

iAValueType iAAttributeDescriptor::valueType() const
{
	return m_valueType;
}

void iAAttributeDescriptor::resetMinMax()
{
	m_min = std::numeric_limits<double>::max();
	m_max = std::numeric_limits<double>::lowest();
}

void iAAttributeDescriptor::adjustMinMax(double value)
{
	if (value < m_min)
	{
		m_min = value;
	}
	if (value > m_max)
	{
		m_max = value;
	}
}

double iAAttributeDescriptor::min() const
{
	return m_min;
}

double iAAttributeDescriptor::max() const
{
	return m_max;
}

QVariant iAAttributeDescriptor::defaultValue() const
{
	return m_defaultValue;
}

void iAAttributeDescriptor::setDefaultValue(QVariant v)
{
	m_defaultValue = v;
}

QString const & iAAttributeDescriptor::name() const
{
	return m_name;
}

bool iAAttributeDescriptor::isLogScale() const
{
	return m_logarithmic;
}

void iAAttributeDescriptor::setLogScale(bool l)
{
	m_logarithmic = l;
}

bool iAAttributeDescriptor::coversWholeRange(double min, double max) const
{
	return min <= m_min && m_max <= max;
}

QSharedPointer<iANameMapper> iAAttributeDescriptor::nameMapper() const
{
	return m_nameMapper;
}

void iAAttributeDescriptor::setNameMapper(QSharedPointer<iANameMapper> mapper)
{
	m_nameMapper = mapper;
}
