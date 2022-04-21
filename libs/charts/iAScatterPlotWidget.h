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

#ifdef CHART_OPENGL
#include <QOpenGLWidget>
using iAChartParentWidget = QOpenGLWidget;
#include <QOpenGLFunctions>
#else
#include <QWidget>
using iAChartParentWidget = QWidget;
#endif

#include "iAScatterPlot.h"	// for iAScatterPlot::SelectionMode, iAScatterPlot::HighlightDrawMode

#include "iAcharts_export.h"

class iAColorTheme;
class iAScatterPlotViewData;
class iASPLOMData;

class QMenu;

//! class for providing information on a point in the scatter plot (used in tooltips)
class iAcharts_API iAScatterPlotPointInfo
{
public:
	virtual ~iAScatterPlotPointInfo();
	virtual QString text(const size_t paramIdx[2], size_t pointIdx) =0;
};

//! Widget for using a single scatter plot (outside of a SPLOM)
// TODO: minimize duplication between iAScatterPlotWidget and iAQSplom!
class iAcharts_API iAScatterPlotWidget : public iAChartParentWidget
#ifdef CHART_OPENGL
	, public QOpenGLFunctions
#endif
{
	Q_OBJECT
public:
	static const int PaddingTop;
	static const int PaddingRight;
	int PaddingBottom();
	int PaddingLeft();
	static const int TextPadding;
	iAScatterPlotWidget();
	iAScatterPlotWidget(QSharedPointer<iASPLOMData> data, bool columnSelection = false);
	void initWidget();
	iASPLOMData * data();
	void setData(QSharedPointer<iASPLOMData> data);
	void setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIdx);
	QSharedPointer<iALookupTable> lookupTable() const;
	void setPlotColor(QColor const & c, double rangeMin, double rangeMax);
	void setSelectionColor(QColor const & c);
	void setSelectionMode(iAScatterPlot::SelectionMode mode);
	void setPickedPointFactor(double factor);
	void setPointRadius(double pointRadius);
	void setFixPointsEnabled(bool enabled);
	void setPointInfo(QSharedPointer<iAScatterPlotPointInfo> pointInfo);
	void toggleHighlightedPoint(size_t curPoint, Qt::KeyboardModifiers modifiers);
	void setHighlightColor(QColor hltCol);
	void setHighlightColorTheme(iAColorTheme const* theme);
	void setHighlightDrawMode(iAScatterPlot::HighlightDrawModes drawMode);
	void setSelectionEnabled(bool enabled);
	//! proxy method for setting visible parameters in contained iAScatterPlot
	void setVisibleParameters(size_t p1, size_t p2);
	void setDrawGridLines(bool enabled);

	void setXMarker(double xPos, QColor const& color, Qt::PenStyle penStyle)
	{
		m_xMarker[xPos] = qMakePair(color, penStyle);
	}

	void removeXMarker(double xPos)
	{
		m_xMarker.remove(xPos);
	}

	void clearMarkers()
	{
		m_xMarker.clear();
	}

	QSharedPointer<iAScatterPlotViewData> viewData();
	const size_t* paramIndices() const;  //!< Get column indices of visible X and Y parameters in data table
protected:
#ifdef CHART_OPENGL
	void initializeGL() override;
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
	void contextMenuEvent(QContextMenuEvent* event) override;
private:
	void adjustScatterPlotSize();
	void drawTooltip(QPainter& painter);
	void currentPointUpdated(size_t index);  //!< When hovered over a new point.

	QSharedPointer<iAScatterPlot> m_scatterplot;
	QSharedPointer<iASPLOMData> m_data;
	QSharedPointer<iAScatterPlotViewData> m_viewData;
	int m_fontHeight = 0,
		m_maxTickLabelWidth = 0;
	bool m_fixPointsEnabled = false,
		m_columnSelection = false;
	QSharedPointer<iAScatterPlotPointInfo> m_pointInfo;
	QMenu *m_contextMenu = nullptr,    //!< the context menu for picking the two visible parameters
		*m_xMenu = nullptr,
		*m_yMenu = nullptr;
	QMap<double, QPair<QColor, Qt::PenStyle>> m_xMarker;

signals:
	//! emitted for each single point that was highlighted (or un-highlighted)
	//! The parameters reflect the new highlight state for the given point.
	//! Note: When this function is called, the highlightedPoints() might
	//! not be at the most up-to-date state (e.g. ptIdx might still be
	//! contained in the returned vector, if state parameter here is false).
	//! If you need a consistent, up-to-date state of all highlighted points,
	//! attach to the highlightChanged signal instead!
	void pointHighlighted(size_t ptIdx, bool state);
	//! Emitted once after the internal state of highlights has been fully updated
	//! to represent the new highlight situation after a user interaction.
	void highlightChanged();
	void selectionModified();
	void visibleParamChanged();
	void chartClicked(double x, double y, Qt::KeyboardModifiers	modifiers);  //!< Emitted when a point in the chart is clicked (and no selection or fixed point selection happened)
private slots:
	void xParamChanged();
	void yParamChanged();
	void updateFilter();
};
