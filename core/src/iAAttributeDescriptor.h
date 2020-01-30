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
#pragma once

#include "iAValueType.h"
#include "open_iA_Core_export.h"

#include <QSharedPointer>
#include <QString>
#include <QVariant>

class iANameMapper;

// TODO: split up into different subtypes according to iAValueType?
// then e.g. iAFilter::CheckParameter could be part of this
// and it would be easier to incorporate type-specific restraints (e.g. a Folder parameter that can also be empty)
class open_iA_Core_API iAAttributeDescriptor
{
public:
	static const QString ValueSplitString;
	enum iAAttributeType
	{
		None = -1,
		Parameter,
		DerivedOutput
	};
	static QSharedPointer<iAAttributeDescriptor> create(QString const & def);
	static QSharedPointer<iAAttributeDescriptor> createParam(
		QString const & name, iAValueType valueType,
		QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(),
		double max = std::numeric_limits<double>::max());
	iAAttributeDescriptor(QString const & name, iAAttributeType attribType, iAValueType valueType);
	virtual ~iAAttributeDescriptor();
	iAAttributeType attribType() const;
	iAValueType valueType() const;
	QVariant defaultValue() const;
	void setDefaultValue(QVariant v);
	virtual QSharedPointer<iANameMapper> nameMapper() const;
	double min() const;
	double max() const;
	QString const & name() const;
	void setLogScale(bool l);
	bool isLogScale() const;
	void resetMinMax();
	void adjustMinMax(double value);
	bool coversWholeRange(double min, double max) const;
	QString toString() const;
	void setNameMapper(QSharedPointer<iANameMapper> mapper);
private:
	iAAttributeDescriptor(iAAttributeDescriptor const & other) = delete;
	iAAttributeDescriptor& operator=(iAAttributeDescriptor const & other) = delete;
	iAAttributeType m_attribType;
	iAValueType m_valueType;
	double m_min, m_max;
	QVariant m_defaultValue;
	bool m_logarithmic;
	QString m_name;
	QSharedPointer<iANameMapper> m_nameMapper;
};
