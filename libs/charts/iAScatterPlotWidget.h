/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#ifdef CHART_OPENGL
#include "iAQGLWidget.h"
using iAChartParentWidget = iAQGLWidget;
#else
#include <QWidget>
using iAChartParentWidget = QWidget;
#endif

#include "iAScatterPlot.h"	// for iAScatterPlot::SelectionMode

#include "iAcharts_export.h"

class iAScatterPlotViewData;
class iASPLOMData;

//! Widget for using a single scatter plot (outside of a SPLOM)
class iAcharts_API iAScatterPlotWidget : public iAChartParentWidget
{
public:
	static const int PaddingTop;
	static const int PaddingRight;
	int PaddingBottom();
	int PaddingLeft();
	static const int TextPadding;
	iAScatterPlotWidget(QSharedPointer<iASPLOMData> data);
	void SetPlotColor(QColor const & c, double rangeMin, double rangeMax);
	void SetSelectionColor(QColor const & c);
	void SetSelectionMode(iAScatterPlot::SelectionMode mode);
	QSharedPointer<iAScatterPlotViewData> viewData();
protected:
#ifdef CHART_OPENGL
	void paintGL() override;
#else
	void paintEvent(QPaintEvent* event) override;
#endif
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
public:
	iAScatterPlot* m_scatterplot;
private:
	void adjustScatterPlotSize();
	QSharedPointer<iASPLOMData> m_data;
	QSharedPointer<iAScatterPlotViewData> m_viewData;
	int m_fontHeight, m_maxTickLabelWidth;
	void currentPointUpdated(size_t index);  //!< When hovered over a new point.
};
