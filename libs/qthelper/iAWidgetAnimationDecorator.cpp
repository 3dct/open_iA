// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAWidgetAnimationDecorator.h"

#include <QPropertyAnimation>

iAWidgetAnimationDecorator::iAWidgetAnimationDecorator(QWidget* animatedWidget, int duration, QColor startValue, QColor endValue, QString animatedQssProperty) :
	m_animation(std::make_shared<QPropertyAnimation>(this, "color")),
	m_animatedQssProperty(animatedQssProperty),
	m_animatedWidget(animatedWidget)
{
	connect(m_animation.get(), &QPropertyAnimation::finished, this, &QObject::deleteLater);
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
