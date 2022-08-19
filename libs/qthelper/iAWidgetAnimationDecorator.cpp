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
#include "iAWidgetAnimationDecorator.h"

#include <QPropertyAnimation>

iAWidgetAnimationDecorator::iAWidgetAnimationDecorator(QWidget* animatedWidget, int duration, QColor startValue, QColor endValue, QString animatedQssProperty) :
	m_animation(QSharedPointer<QPropertyAnimation>::create(this, "color")),
	m_animatedQssProperty(animatedQssProperty),
	m_animatedWidget(animatedWidget)
{
	connect(m_animation.data(), &QPropertyAnimation::finished, this, &QObject::deleteLater);
	connect(m_animatedWidget, &QWidget::destroyed, this, [this]
	{
		m_animation->stop();
		//deleteLater();
	});
	m_animation->setDuration(duration);
	m_animation->setStartValue(startValue);
	m_animation->setEndValue(endValue);
	m_animation->start();
}

void iAWidgetAnimationDecorator::animate(QWidget* animatedWidget, int duration, QColor startValue, QColor endValue, QString animatedQssProperty)
{
	new iAWidgetAnimationDecorator(animatedWidget, duration, startValue, endValue, animatedQssProperty);
}

iAWidgetAnimationDecorator::~iAWidgetAnimationDecorator()
{
}

void iAWidgetAnimationDecorator::setColor(QColor color)
{
	m_animatedWidget->setStyleSheet(QString("%1: rgb(%2, %3, %4);").arg(m_animatedQssProperty).arg(color.red()).arg(color.green()).arg(color.blue()));
}

QColor iAWidgetAnimationDecorator::color() const
{
	return Qt::black; // getter is not really needed for now
}
