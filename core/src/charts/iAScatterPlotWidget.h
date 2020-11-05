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

#include "qthelper/iAQGLWidget.h"

#include "iAScatterPlot.h"	// for iAScatterPlot::SelectionMode

class iASPLOMData;
class iAScatterPlotStandaloneHandler;

//! class for providing information on a point in the scatter plot (used in tooltips)
class iAScatterPlotPointInfo
{
public:
	virtual QString text(const size_t paramIdx[2], size_t pointIdx) =0;
};

//! Widget for using a single scatter plot (outside of a SPLOM)
// TODO: minimize duplication between iAScatterPlotWidget and iAQSplom!
class open_iA_Core_API iAScatterPlotWidget : public iAQGLWidget
{
	Q_OBJECT
public:
	static const int PaddingTop;
	static const int PaddingRight;
	int PaddingBottom();
	int PaddingLeft();
	static const int TextPadding;
	iAScatterPlotWidget(QSharedPointer<iASPLOMData> data);
	std::vector<size_t> & selection();
	void setSelection(std::vector<size_t> const & selection);
	void setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIdx);
	void setPlotColor(QColor const & c, double rangeMin, double rangeMax);
	void setSelectionColor(QColor const & c);
	void setSelectionMode(iAScatterPlot::SelectionMode mode);
	void setPointRadius(double pointRadius);
	void setFixPointsEnabled(bool enabled);
	void setPointInfo(QSharedPointer<iAScatterPlotPointInfo> pointInfo);
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
	void drawTooltip(QPainter& painter);

	QSharedPointer<iASPLOMData> m_data;
	QSharedPointer<iAScatterPlotStandaloneHandler> m_scatterPlotHandler;
	int m_fontHeight, m_maxTickLabelWidth;
	bool m_fixPointsEnabled;
	QSharedPointer<iAScatterPlotPointInfo> m_pointInfo;
signals:
	void pointHighlighted(size_t ptIdx, bool state);
};
