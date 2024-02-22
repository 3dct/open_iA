// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

#include "iacharts_export.h"

class iAColorTheme;
class iAScatterPlotViewData;
class iASPLOMData;

class QMenu;

//! Provides information on a point in the scatter plot (used in tooltips).
class iAcharts_API iAScatterPlotPointInfo
{
public:
	virtual ~iAScatterPlotPointInfo();
	virtual QString text(const size_t paramIdx[2], size_t pointIdx) =0;
};

//! Widget for using a single scatter plot (outside of a SPLOM).
//! @todo minimize duplication between iAScatterPlotWidget and iAQSplom!
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
	iAScatterPlotWidget(std::shared_ptr<iASPLOMData> data, bool columnSelection = false);
	void initWidget();
	iASPLOMData * data();
	void setData(std::shared_ptr<iASPLOMData> data);
	void setLookupTable(std::shared_ptr<iALookupTable> lut, size_t paramIdx);
	std::shared_ptr<iALookupTable> lookupTable() const;
	void setPlotColor(QColor const & c, double rangeMin, double rangeMax);
	void setSelectionColor(QColor const & c);
	void setSelectionMode(iAScatterPlot::SelectionMode mode);
	void setPickedPointFactor(double factor);
	void setPointRadius(double pointRadius);
	void setFixPointsEnabled(bool enabled);
	void setShowToolTips(bool enabled);
	void setPointInfo(std::shared_ptr<iAScatterPlotPointInfo> pointInfo);
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
	double const* yBounds() const;
	void setYBounds(double yMin, double yMax);
	void resetYBounds();

	std::shared_ptr<iAScatterPlotViewData> viewData();
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

	std::shared_ptr<iAScatterPlot> m_scatterplot;
	std::shared_ptr<iASPLOMData> m_data;
	std::shared_ptr<iAScatterPlotViewData> m_viewData;
	int m_fontHeight = 0,
		m_maxTickLabelWidth = 0;
	bool m_fixPointsEnabled = false,
		m_columnSelection = false,
		m_showTooltip = true;
	std::shared_ptr<iAScatterPlotPointInfo> m_pointInfo;
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
	void chartPress(double x, double y, Qt::KeyboardModifiers modifiers);  //!< Emitted when the mouse is pressed in the chart (and no selection or fixed point selection happened)
private slots:
	void xParamChanged();
	void yParamChanged();
	void updateFilter();
};
