/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iAValueType.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVector>
#include <QWidget>

//! Input line for a 2- or 3 dimensional vector with either continuous (double) or discrete (integer) values.
//! Currently only used in iAParameterDialog. If used somewhere else, extract method
//! definitions to separate .cpp file to avoid according errors
class iAVectorInput : public QWidget
{
	Q_OBJECT
public:
	iAVectorInput(QWidget* parent, iAValueType valueType, int vecLength, QVariant const & values) :
		QWidget(parent), m_inputs(vecLength), m_valueType(valueType)
	{
		setLayout(new QHBoxLayout);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->setSpacing(4);
		for (int i = 0; i < vecLength; ++i)
		{
			layout()->addWidget(new QLabel(ComponentNames[i]));
			if (valueType == iAValueType::Discrete)
			{
				auto in = new QSpinBox(this);
				in->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
				m_inputs[i] = in;
			}
			else
			{
				auto in = new QDoubleSpinBox(this);
				in->setRange(-9999999999, 9999999999);  // To Do: pass in as parameters?
				in->setDecimals(2);
				m_inputs[i] = in;
			}
			layout()->addWidget(m_inputs[i]);
		}
		setValue(values);
	}
	QVariant value() const
	{
		if (m_valueType == iAValueType::Discrete)
		{
			QVector<int> values(m_inputs.size());

			for (int i = 0; i < m_inputs.size(); ++i)
			{
				values[i] = qobject_cast<QSpinBox*>(m_inputs[i])->value();
			}
			return QVariant::fromValue(values);
		}
		else
		{
			QVector<double> values(m_inputs.size());
			for (int i = 0; i < m_inputs.size(); ++i)
			{
				values[i] = qobject_cast<QDoubleSpinBox*>(m_inputs[i])->value();
			}
			return QVariant::fromValue(values);
		}
	}
	void setValue(QVariant const& valueVariant)
	{
		QVector<double> values = valueVariant.value<QVector<double>>();
		assert(values.size() <= m_inputs.size());
		for (int i = 0; i < m_inputs.size() && i < values.size(); ++i)
		{
			if (m_valueType == iAValueType::Discrete)
			{
				qobject_cast<QSpinBox*>(m_inputs[i])->setValue(values[i]);
			}
			else
			{
				 qobject_cast<QDoubleSpinBox*>(m_inputs[i])->setValue(values[i]);
			}
		}
	}

private:
	const QString ComponentNames[3] = {"x", "y", "z"};
	QVector<QAbstractSpinBox*> m_inputs;
	iAValueType m_valueType;
};