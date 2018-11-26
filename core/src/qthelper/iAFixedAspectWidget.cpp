/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <QVTKOpenGLWidget.h>

#include <QPainter>
#include <QVBoxLayout>

class iAColoredWidget: public QWidget
{
public:
	iAColoredWidget(QColor const & c):
	    bgColor(c)
	{
	}
	void paintEvent(QPaintEvent* ev)
	{
		QPainter p(this);
		QRect g(geometry());
		p.fillRect(g, bgColor);
	}
private:
	QColor bgColor;
};

class iAFixedAspectWidgetInternal: public QVTKOpenGLWidget
{
public:
	iAFixedAspectWidgetInternal(double aspect):
		m_aspect(aspect)
	{
	}
	bool hasHeightForWidth() const override
	{
		return true;
	}
	int heightForWidth(int w) const override
	{
		int newHeight = static_cast<int>(w*m_aspect);
		DEBUG_LOG(QString("width x height: %1 x %2").arg(w).arg(newHeight));
		return newHeight;
	}
private:
	double m_aspect;
	QWidget* m_child;
};

iAFixedAspectWidget::iAFixedAspectWidget(double aspect, Qt::Alignment verticalAlign)
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(0, 0, 0, 0);
	if (verticalAlign != Qt::AlignTop)
	{
		iAColoredWidget* w = new iAColoredWidget(QColor(255, 0, 0, 50));
		w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout()->addWidget(w);
	}
	m_widget = new iAFixedAspectWidgetInternal(aspect);
	layout()->addWidget(m_widget);
	if (verticalAlign != Qt::AlignBottom)
	{
		iAColoredWidget* filler = new iAColoredWidget(QColor(0, 255, 0, 50));
		filler->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout()->addWidget(filler);
	}
}

QVTKOpenGLWidget* iAFixedAspectWidget::vtkWidget()
{
	return m_widget;
}