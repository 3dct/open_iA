// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAttributeDescriptor.h"

#include "iALog.h"
#include "iAListNameMapper.h"
#include "iAStringHelper.h"    // for iAConverter

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
		LOG(lvlWarn, QString("Unknown attribute descriptor '%1'\n").arg(str));
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
		LOG(lvlWarn, QString("Not enough tokens in attribute descriptor '%1'").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	auto result = QSharedPointer<iAAttributeDescriptor>::create(
			defTokens[0], Str2AttribType(defTokens[1]), Str2ValueType(defTokens[2])
	);
	int requiredTokens = (result->valueType() == iAValueType::Boolean) ? 3 :
		((result->valueType() == iAValueType::Categorical) ? 4 : 5);
	if (defTokens.size() < requiredTokens)
	{
		LOG(lvlWarn, QString("Not enough tokens in attribute descriptor '%1'").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	if (result->valueType() == iAValueType::Continuous || result->valueType() == iAValueType::Discrete)
	{
		result->m_logarithmic = false;
		bool minOk = true, maxOk = true;
		result->m_min = iAConverter<double>::toT(defTokens[3], &minOk);
		result->m_max = iAConverter<double>::toT(defTokens[4], &maxOk);
		if (!minOk || !maxOk)
		{
			LOG(lvlWarn, QString("Minimum or maximum of attribute couldn't be parsed in line %1\n").arg(def));
			return QSharedPointer<iAAttributeDescriptor>();
		}
		if (defTokens.size() >= 6)
		{
			result->m_logarithmic = (defTokens[5] == LogarithmicStr);
		}
		if (defTokens.size() > 6)
		{
			LOG(lvlWarn, QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
		}
	}
	else if (result->valueType() == iAValueType::Categorical)
	{
		QStringList categories = defTokens[3].split(CategoricalValueSplitString);
		result->m_min = 0;
		result->m_max = categories.size()-1;
		result->m_defaultValue = categories;
		if (defTokens.size() > 5)
		{
			LOG(lvlWarn, QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
		}
	}
	return result;
}

QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::createParam(
	QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	auto result = QSharedPointer<iAAttributeDescriptor>::create(name, Parameter, valueType);
	result->m_min = valueType == iAValueType::Categorical ? 0 : min;
	result->m_max = valueType == iAValueType::Categorical ? defaultValue.toStringList().size()-1 : max;
	result->m_defaultValue = defaultValue;
	return result;
}

QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::clone() const
{
	auto result = QSharedPointer<iAAttributeDescriptor>::create(m_name, m_attribType, m_valueType);
	result->m_min = m_min;
	result->m_max = m_max;
	result->m_defaultValue = m_defaultValue;
	result->m_logarithmic = m_logarithmic;
	result->m_nameMapper = m_nameMapper;
	return result;
}

QString iAAttributeDescriptor::toString() const
{
	QString result = name() + AttributeSplitString +
		AttribType2Str(attribType()) + AttributeSplitString +
		ValueType2Str(valueType()) + AttributeSplitString;
	if (valueType() == iAValueType::Continuous || valueType() == iAValueType::Discrete)
	{
		result += QString::number(min()) + AttributeSplitString + QString::number(max()) + AttributeSplitString + (m_logarithmic ? LogarithmicStr : LinearStr);
	}
	else if (valueType() == iAValueType::Categorical)
	{
		result += m_defaultValue.toStringList().join(CategoricalValueSplitString);
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
	if (valueType() == iAValueType::Categorical)
	{
		m_min = 0;
		m_max = v.toStringList().size() - 1;
	}
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
	if (!m_nameMapper)
	{
		m_nameMapper = QSharedPointer<iAListNameMapper>::create(m_defaultValue.toStringList());
	}
	return m_nameMapper;
}

void selectOption(QStringList& options, QString const& selected)
{
	for (int i = 0; i < options.size(); ++i)
	{
		if (options[i].startsWith("!"))  // optimization: check if option with ! removed equals selected
		{                                   // then entry is already selected, we wouldn't need to remove and add again
			options[i].remove(0, 1);
		}
		if (options[i].compare(selected, Qt::CaseInsensitive) == 0)
		{
			options[i] = "!" + options[i];
		}
	}
}
