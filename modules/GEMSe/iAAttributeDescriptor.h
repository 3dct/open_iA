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

#pragma once

#include "iAValueType.h"

#include <QSharedPointer>
#include <QString>

class iANameMapper;

class iAAttributeDescriptor
{
public:
	enum iAAttributeType
	{
		Invalid = -1,
		Parameter,
		DerivedOutput
	};
	static QSharedPointer<iAAttributeDescriptor> Create(QString const & def);
	iAAttributeDescriptor(QString const & name, iAAttributeType attribType, iAValueType valueType);
	iAAttributeType GetAttribType() const;
	iAValueType GetValueType() const;
	virtual QSharedPointer<iANameMapper> GetNameMapper() const;
	double GetMin() const;
	double GetMax() const;
	QString GetName() const;
	void SetLogScale(bool l);
	bool IsLogScale() const;
	void ResetMinMax();
	void AdjustMinMax(double value);
	bool CoversWholeRange(double min, double max) const;
	QString ToString() const;
private:
	iAAttributeType m_attribType;
	iAValueType m_valueType;
	double m_min, m_max;
	bool m_logarithmic;
	QString m_name;
	QSharedPointer<iANameMapper> m_nameMapper;
};
