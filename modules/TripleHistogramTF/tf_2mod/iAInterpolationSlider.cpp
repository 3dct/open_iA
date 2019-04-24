/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAInterpolationSlider.h"

#include <QResizeEvent>
#include <QLabel>
#include <QVBoxLayout>

//iAInterpolationSlider::iAInterpolationSlider(Qt::Orientation orientation, QWidget* parent)
iAInterpolationSlider::iAInterpolationSlider(QWidget* parent)
{
	m_labelA = new QLabel("100%");
	m_labelB = new QLabel("100%");

	m_labelA->setMinimumWidth(50);
	m_labelB->setMinimumWidth(50);

	m_slider = new QSlider(Qt::Vertical, parent);
	m_slider->setInvertedAppearance(true);
	m_slider->setMinimum(0);
	m_slider->setMaximum(100 * pow(10, 3)); // 3 decimal precision

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_labelA, 0);
	layout->addWidget(m_slider, 1);
	layout->addWidget(m_labelB, 0);

	setT(0.5);

	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(slider_valueChanged(int)));
}

void iAInterpolationSlider::setT(double t)
{
	t = t > 1 ? 1 : (t < 0 ? 0 : t); // Make sure t is in range [0,1] (if not, clamp it)
	int range = m_slider->maximum() - m_slider->minimum();
	int value = t * range + m_slider->minimum();
	m_slider->setValue(value);
	setTPrivate(t);
}

void iAInterpolationSlider::setTPrivate(double t)
{
	m_t = t;

	int a = (1 - t) * 100;
	int b = 100 - a;

	m_labelA->setText(QString::number(a) + "%");
	m_labelB->setText(QString::number(b) + "%");

	emit tChanged(t);
}

void iAInterpolationSlider::setValuePrivate(int v)
{
	// m_t is always in the range [0, 1]
	double t = (double)v / (double)m_slider->maximum();
	setTPrivate(t);
}

void iAInterpolationSlider::slider_valueChanged(int v)
{
	setValuePrivate(v);
}