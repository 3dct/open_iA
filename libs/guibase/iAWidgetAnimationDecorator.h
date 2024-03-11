// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QApplication>
#include <QWidget>

class QPropertyAnimation;

//! Animates a given widget property.
//! inspired from https://stackoverflow.com/a/34445886
class iAguibase_API iAWidgetAnimationDecorator: public QObject
{
	Q_OBJECT
	Q_PROPERTY(QColor color READ color WRITE setColor)
public:
	~iAWidgetAnimationDecorator();
	void setColor(QColor color);
	QColor color() const;
	static void animate(QWidget* animatedWidget,
		int duration = 2000,
		QColor startValue = QColor(255, 0, 0),
		QColor endValue = QApplication::palette().color(QPalette::Window),
		QString animatedQssProperty = "background-color");
private:
	iAWidgetAnimationDecorator(QWidget* animatedWidget, int duration,
		QColor startValue, QColor endValue, QString animatedQssProperty);
	std::shared_ptr<QPropertyAnimation> m_animation;
	QString m_animatedQssProperty;
	QWidget* m_animatedWidget;
};
