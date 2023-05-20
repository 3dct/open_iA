// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAValueType.h"

#include "iabase_export.h"

#include <QSharedPointer>
#include <QString>
#include <QVariant>

class iANameMapper;

// TODO: split up into different subtypes according to iAValueType?
// then e.g. iAFilter::CheckParameter could be part of this
// and it would be easier to incorporate type-specific restraints (e.g. a Folder parameter that can also be empty)
//! Class for describing an attribute or parameter: its value type (see iAValueType), its name, its range, etc.
class iAbase_API iAAttributeDescriptor
{
public:
	//! Type of attribute (parameter or derived output)
	enum iAAttributeType
	{
		None = -1,
		Parameter,
		DerivedOutput
	};
	//! Create an attribute descriptor from the given string
	//! (which could e.g. be created by the toString method).
	static QSharedPointer<iAAttributeDescriptor> create(QString const & def);
	//! Create an attribute descriptor with attribute type Parameter from the given values
	static QSharedPointer<iAAttributeDescriptor> createParam(
		QString const & name, iAValueType valueType,
		QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(),
		double max = std::numeric_limits<double>::max());
	//! Create an attribute descriptor.
	iAAttributeDescriptor(QString const & name, iAAttributeType attribType, iAValueType valueType);
	virtual ~iAAttributeDescriptor();
	//! Deep copy the attribute descriptor.
	QSharedPointer<iAAttributeDescriptor> clone() const;
	//! The type of attribute (parameter or derived output, see iAAttributeType).
	iAAttributeType attribType() const;
	//! The type of the value of this attribute; see iAValueType.
	iAValueType valueType() const;
	//! (default) value of the attribute. For categorical parameters, this QVariant holds a QStringList
	//! (access via QVariant::toStringList()) of all possible values.
	QVariant defaultValue() const;
	//! set the (default) values of the attribute. For categorical parameters, this should be a
	//! QStringList of all possible values.
	void setDefaultValue(QVariant v);
	virtual QSharedPointer<iANameMapper> nameMapper() const;
	//! For discrete/continuous types, the minimum possible value. For categorical values, it's 0.
	double min() const;
	//! For discrete/continuous types, the maximum possible value. For categorical values,
	//! it's (number of of possible values) - 1.
	double max() const;
	//! The name of the attribute.
	QString const & name() const;
	//! Set whether the attribute is given in a logarithmic scale.
	void setLogScale(bool l);
	//! Check whether the attribute is given in a logarithmic scale.
	bool isLogScale() const;
	//! reset minimum/maximum to highest/lowest values representable by double.
	void resetMinMax();
	//! adjust minimum/maximum so that the given value fits inside
	//! @param value if smaller than current minimum, it will be the new minimum;
	//!     if greater than current maximum, it will be the new maximum.
	void adjustMinMax(double value);
	//! check whether the given interval covers the whole internal min/max range
	//! @return true if min smaller or equal internal minimum and maximum greater or equal internal maximum
	bool coversWholeRange(double min, double max) const;
	//! Convert the attribute descriptor to a string.
	QString toString() const;
private:
	iAAttributeDescriptor(iAAttributeDescriptor const & other) = delete;
	iAAttributeDescriptor& operator=(iAAttributeDescriptor const & other) = delete;
	iAAttributeType m_attribType;
	iAValueType m_valueType;
	double m_min, m_max;
	QVariant m_defaultValue;
	bool m_logarithmic;
	QString m_name;
	mutable QSharedPointer<iANameMapper> m_nameMapper;
};

iAbase_API void selectOption(QStringList& options, QString const& selected);
iAbase_API QString selectedOption(QStringList const& options);
