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

#include "iAAttributeDescriptor.h"

#include "iAConsole.h"
#include "iAGEMSeConstants.h"

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

iAValueType Str2ValueType(QString const & str)
{
	if (str == ContinuousStr)
	{
		return iAValueType::Continuous;
	}
	else if (str == DiscreteStr)
	{
		return iAValueType::Discrete;
	}
	else if (str == CategoricalStr)
	{
		return iAValueType::Categorical;
	}
	else
	{
		DEBUG_LOG(QString("Unknown value type '%1'\n").arg(str));
		return iAValueType::Invalid;
	}
}

QString ValueType2Str(iAValueType type)
{
	switch (type)
	{
	case iAValueType::Continuous:
		return ContinuousStr;
	case iAValueType::Discrete:
		return DiscreteStr;
	case iAValueType::Categorical:
		return CategoricalStr;
	default:
		return UnknownStr;
	}
}

#include "iANameMapper.h"

#include <QStringList>

class iAListNameMapper : public iANameMapper
{
public:
	iAListNameMapper(QStringList const & names) :
		m_names(names)
	{}
	virtual QString GetName(int idx) const
	{
		return m_names[idx];
	}

	virtual int GetIdx(QString const & name, bool & ok) const
	{
		for (int i = 0; i < m_names.size(); ++i)
		{
			if (m_names[i] == name)
			{
				ok = true;
				return i;
			}
		}
		ok = false;
		return -1;
	}

	virtual int size() const
	{
		return m_names.size();
	}
private:
	QStringList m_names;
};


QSharedPointer<iAAttributeDescriptor> iAAttributeDescriptor::Create(QString const & def)
{
	QStringList defTokens = def.split(AttributeSplitString);
	assert(defTokens.size() >= 3);
	if (defTokens.size() < 3)
	{
		DEBUG_LOG(QString("Not enough tokens in attribute descriptor %1").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	QString name = defTokens[0];
	
	QSharedPointer<iAAttributeDescriptor> result(
		new iAAttributeDescriptor(
			name, Str2AttribType(defTokens[1]), Str2ValueType(defTokens[2])
		)
	);
	int requiredTokens = result->GetValueType() == Categorical ? 4 : 5;
	if (defTokens.size() < requiredTokens)
	{
		DEBUG_LOG(QString("Not enough tokens in continuous attribute descriptor %1").arg(def));
		return QSharedPointer<iAAttributeDescriptor>();
	}
	switch (result->GetValueType())
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
			{
				result->m_logarithmic = (defTokens[5] == LogarithmicStr);
			}
			if (defTokens.size() > 6)
			{
				DEBUG_LOG(QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
			}
			break;
		}
		case Categorical:
		{
			QStringList categories = defTokens[3].split(CategoricalValueSplitString);
			result->m_min = 0;
			result->m_max = categories.size()-1;
			result->m_nameMapper = QSharedPointer<iAListNameMapper>(new iAListNameMapper(categories));
			if (defTokens.size() > 5)
			{
				DEBUG_LOG(QString("Superfluous tokens in attribute descriptor %1\n").arg(def));
			}
			break;
		}
	}
	return result;
}


QString iAAttributeDescriptor::ToString() const
{
	QString result = GetName() + AttributeSplitString +
		AttribType2Str(GetAttribType()) + AttributeSplitString +
		ValueType2Str(GetValueType()) + AttributeSplitString;
	switch (GetValueType())
	{
		case iAValueType::Continuous: // intentional fall-through!
		case iAValueType::Discrete:
			result += QString::number(GetMin()) + AttributeSplitString + QString::number(GetMax()) + AttributeSplitString + (m_logarithmic ? LogarithmicStr : LinearStr);
			break;
		case iAValueType::Categorical:	{
			if (!m_nameMapper)
			{
				DEBUG_LOG("NameMapper NULL for categorical attribute!\n");
				for (int i = GetMin(); i <= GetMax(); ++i)
				{
					result += QString::number(i);
					if (i < GetMax()) result += CategoricalValueSplitString;
				}
				break;
			}
			for (int i = GetMin(); i <= GetMax(); ++i)
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

iAAttributeDescriptor::iAAttributeType iAAttributeDescriptor::GetAttribType() const
{
	return m_attribType;
}

iAValueType iAAttributeDescriptor::GetValueType() const
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

double iAAttributeDescriptor::GetMin() const
{
	return m_min;
}

double iAAttributeDescriptor::GetMax() const
{
	return m_max;
}

QString iAAttributeDescriptor::GetName() const
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

QSharedPointer<iANameMapper> iAAttributeDescriptor::GetNameMapper() const
{
	return m_nameMapper;
}

void iAAttributeDescriptor::SetNameMapper(QSharedPointer<iANameMapper> mapper)
{
	m_nameMapper = mapper;
}
