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
#include "iAAttributeDescriptor.h"

#include "iAConsole.h"
#include "iAListNameMapper.h"

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

#include <cassert>

template <typename T>
struct Converter
{
	static T convert(QString str, bool * ok)
	{
		DEBUG_LOG("Unspecialized Converter called! This should not happen!\n");
		assert(false);
		return std::numeric_limits<T>::signaling_NaN();
	}
};

template <>
struct Converter<int>
{
	static int convert(QString str, bool * ok)
	{
		return str.toInt(ok);
	}
};

template <>
struct Converter<double>
{
	static double convert(QString str, bool * ok)
	{
		return str.toDouble(ok);
	}
};


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

QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::Create(QString const & def)
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
	int requiredTokens = (result->ValueType() == Boolean) ? 3 :
		((result->ValueType() == Categorical) ? 4 : 5);
	if (defTokens.size() < requiredTokens)
	{
		DEBUG_LOG(QString("Not enough tokens in attribute descriptor '%1'").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	switch (result->ValueType())
	{
		case Continuous:	// intentional fall-through!
		case Discrete:
		{
			result->m_logarithmic = false;
			bool minOk = true, maxOk = true;
			result->m_min = Converter<double>::convert(defTokens[3], &minOk);
			result->m_max = Converter<double>::convert(defTokens[4], &maxOk);
			if (!minOk || !maxOk)
			{
				DEBUG_LOG(QString("Minimum or maximum of attribute couldn't be parsed in line %1\n").arg(def));
				return QSharedPointer<iAAttributeDescriptor>();
			}
			if (defTokens.size() >= 6)
				result->m_logarithmic = (defTokens[5] == LogarithmicStr);
			if (defTokens.size() > 6)
				DEBUG_LOG(QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
			break;
		}
		case Categorical:
		{
			QStringList categories = defTokens[3].split(CategoricalValueSplitString);
			result->m_min = 0;
			result->m_max = categories.size()-1;
			result->m_nameMapper = QSharedPointer<iAListNameMapper>(new iAListNameMapper(categories));
			if (defTokens.size() > 5)
				DEBUG_LOG(QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
			break;
		}
	}
	return result;
}

QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::CreateParam(
	QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	auto result = QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(name, Parameter, valueType));
	result->m_min = min;
	result->m_max = max;
	result->m_defaultValue = defaultValue;
	return result;
}


QString iAAttributeDescriptor::ToString() const
{
	QString result = Name() + AttributeSplitString +
		AttribType2Str(AttribType()) + AttributeSplitString +
		ValueType2Str(ValueType()) + AttributeSplitString;
	switch (ValueType())
	{
		case iAValueType::Continuous: // intentional fall-through!
		case iAValueType::Discrete:
			result += QString::number(Min()) + AttributeSplitString + QString::number(Max()) + AttributeSplitString + (m_logarithmic ? LogarithmicStr : LinearStr);
			break;
		case iAValueType::Categorical:	{
			if (!m_nameMapper)
			{
				DEBUG_LOG("NameMapper NULL for categorical attribute!\n");
				for (int i = Min(); i <= Max(); ++i)
				{
					result += QString::number(i);
					if (i < Max()) result += CategoricalValueSplitString;
				}
				break;
			}
			for (int i = Min(); i <= Max(); ++i)
			{
				result += m_nameMapper->GetName(i);
				if (i < m_nameMapper->size() - 1)
				{
					result += CategoricalValueSplitString;
				}
			}
			break;
		}
	}
	return result + "\n";
}

iAAttributeDescriptor::iAAttributeDescriptor(
		QString const & name, iAAttributeType attribType, iAValueType valueType) :
	m_name(name),
	m_attribType(attribType),
	m_valueType(valueType),
	m_logarithmic(false)
{
	ResetMinMax();	// TODO: check why we set it in constructor when it's reset again here anyway; maybe move this to where it's actually needed?
}

iAAttributeDescriptor::iAAttributeType iAAttributeDescriptor::AttribType() const
{
	return m_attribType;
}

iAValueType iAAttributeDescriptor::ValueType() const
{
	return m_valueType;
}

void iAAttributeDescriptor::ResetMinMax()
{
	m_min = std::numeric_limits<double>::max();
	m_max = std::numeric_limits<double>::lowest();
}

void iAAttributeDescriptor::AdjustMinMax(double value)
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

double iAAttributeDescriptor::Min() const
{
	return m_min;
}

double iAAttributeDescriptor::Max() const
{
	return m_max;
}

QVariant iAAttributeDescriptor::DefaultValue() const
{
	return m_defaultValue;
}

QString iAAttributeDescriptor::Name() const
{
	return m_name;
}

bool iAAttributeDescriptor::IsLogScale() const
{
	return m_logarithmic;
}


void iAAttributeDescriptor::SetLogScale(bool l)
{
	m_logarithmic = l;
}

bool iAAttributeDescriptor::CoversWholeRange(double min, double max) const
{
	return min <= m_min && m_max <= max;
}

QSharedPointer<iANameMapper> iAAttributeDescriptor::NameMapper() const
{
	return m_nameMapper;
}

void iAAttributeDescriptor::SetNameMapper(QSharedPointer<iANameMapper> mapper)
{
	m_nameMapper = mapper;
}
