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
#include "iAFixedAspectWidget.h"

#include "iAConsole.h"
#include "iAVtkQtWidget.h"

#include <QVTKOpenGLWidget.h>

#include <QPainter>
#include <QVBoxLayout>

//! Widget with a fixed colored background color.
class iAColoredWidget: public QWidget
{
public:
	iAColoredWidget()
	{}
	void paintEvent(QPaintEvent* /*ev*/) override
	{
		QPainter p(this);
		if (!bgColor.isValid())
			bgColor = QWidget::palette().color(QWidget::backgroundRole());
		p.fillRect(rect(), bgColor);
	}
	void setBGColor(QColor const & color)
	{
		bgColor = color;
		update();
	}
private:
	QColor bgColor;
};

//! The internal iAVtkQtWidget of an iAFixedAspectWidget which actually keeps its aspect ratio.
class iAFixedAspectWidgetInternal: public iAVtkQtWidget
{
public:
	iAFixedAspectWidgetInternal(double aspect):
		m_aspect(aspect)
	{}
	bool hasHeightForWidth() const override
	{
		return true;
	}
	int heightForWidth(int w) const override
	{
		int newHeight = static_cast<int>(w*m_aspect);
		return newHeight;
	}
private:
	double m_aspect;
};

iAFixedAspectWidget::iAFixedAspectWidget(double aspect, Qt::Alignment verticalAlign):
	m_fill1(nullptr),
	m_fill2(nullptr)
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(0, 0, 0, 0);
	if (verticalAlign != Qt::AlignTop)
	{
		m_fill1 = new iAColoredWidget();
		m_fill1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout()->addWidget(m_fill1);
	}
	m_widget = new iAFixedAspectWidgetInternal(aspect);
	layout()->addWidget(m_widget);
	if (verticalAlign != Qt::AlignBottom)
	{
		m_fill2 = new iAColoredWidget();
		m_fill2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout()->addWidget(m_fill2);
	}
}

iAVtkQtWidget* iAFixedAspectWidget::vtkWidget()
{
	return m_widget;
}

void iAFixedAspectWidget::setBackgroundColor(QColor const & color)
{
	if (m_fill1)
		m_fill1->setBGColor(color);
	if (m_fill2)
		m_fill2->setBGColor(color);
}
