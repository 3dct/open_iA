/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "open_iA_Core_export.h"

#include <QObject>

#include "open_iA_Core_export.h"

class QColor;
class QMouseEvent;
class QPainter;
class iAChartWithFunctionsWidget;

//! Abstract base class for representing some kind of function in an an iAChartWithFunctionsWidget.
class open_iA_Core_API iAChartFunction: public QObject
{
	Q_OBJECT
public:
	static const int LineWidthUnselected = 1;
	static const int LineWidthSelected   = 2;

	iAChartFunction(iAChartWithFunctionsWidget* chart) : m_chart(chart) { }

	virtual void draw(QPainter &painter) = 0;
	virtual void draw(QPainter &painter, QColor color, int lineWidth) = 0;
	virtual void drawOnTop(QPainter &painter) = 0;

	virtual int selectPoint(QMouseEvent *event, int *x = nullptr) = 0;
	virtual int getSelectedPoint() const = 0;
	virtual int addPoint(int mouseX, int mouseY) = 0;
	virtual void addColorPoint(int x, double red = -1.0, double green = -1.0, double blue = -1.0) = 0;
	virtual void removePoint(int index) = 0;
	virtual void moveSelectedPoint(int mouseX, int mouseY) = 0;
	virtual void changeColor(QMouseEvent *event) = 0;

	virtual bool isColored() const = 0;
	virtual bool isEndPoint(int index) const = 0;
	virtual bool isDeletable(int index) const = 0;
	virtual size_t numPoints() const = 0;

	virtual void reset() = 0;
	virtual void mouseReleaseEvent(QMouseEvent *) {}
	virtual void mouseReleaseEventAfterNewPoint(QMouseEvent *) {}

	virtual QString name() const = 0;

	iAChartWithFunctionsWidget* m_chart;
};
