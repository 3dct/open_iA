/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"

#include "iAPlotData.h"
#include "qthelper/iAQGLWidget.h"

#include <vector>

#include <QMap>

class iAPlot;
class iAMapper;

class QHelpEvent;
class QMenu;
class QRubberBand;

//! A chart widget which can show an arbitrary number of plots.
class open_iA_Core_API iAChartWidget : public iAQGLWidget
{
	Q_OBJECT
public:
	static const size_t MaxPossiblePlot = std::numeric_limits<size_t>::max();
	enum Mode { NO_MODE, MOVE_VIEW_MODE, X_ZOOM_MODE, Y_ZOOM_MODE };
	enum SelectionMode { SelectionDisabled, SelectPlot };
	enum AxisMappingType { Linear, Logarithmic };
	iAChartWidget(QWidget* parent, QString const & xLabel, QString const & yLabel);
	~iAChartWidget();
	//! @{ Get x/y zoom and shift.
	double xZoom()  const { return m_xZoom;        }
	double yZoom()  const { return m_yZoom;        }
	int    xShift() const { return m_translationX; }
	int    yShift() const { return m_translationY; }
	//! @}
	//! Retrieve bottom margin (in pixels).
	virtual int bottomMargin() const;
	//! Retrieve left margin (in pixels).
	virtual int leftMargin() const;
	//! Retrieve width (in pixels) of the actual chart, i.e. the region where plots are drawn, without space for margins / axes.
	virtual int chartWidth()  const;
	//! Retrieve height (in pixels) of the actual chart, i.e. the region where plots are drawn, without space for the margins / axes.
	virtual int chartHeight() const;
	//! @{ Retrieve minimum/maximum y data value.
	iAPlotData::DataType minYDataValue(size_t startPlot = 0) const;
	iAPlotData::DataType maxYDataValue(size_t startPlot = 0) const;
	//! @}
	//! @{ Get mapper for x/y coordinates
	iAMapper const & xMapper() const;
	iAMapper const & yMapper() const;
	//! @}
	//! @{ Get x/y bounds as array of size 2 (minimum, maximum)
	virtual iAPlotData::DataType const * yBounds() const;
	virtual double const * xBounds() const;
	//! @}
	//! Whether the axis of this chart is categorical type (as determined by the first plot).
	bool categoricalAxis() const;
	//! Get the range in x direction (i.e. maximum - minimum of x bounds)
	double xRange() const;
	//! Get the maximum zoom factor in x direction that can be in use.
	double maxXZoom() const;
	//! Convert an x screen coordinate to a bin space index;
	//! Note that there are three different spaces to consider: <ol>
	//! <li>the data space (i.e., a coordinate between the minimum/maximum specified by the x bounds),</li>
	//! <li>the bin space (i.e., an index in the data bin array)</li>
	//! <li>the screen space (i.e., a pixel x coordinate on the screen)</li></ol>
	//! @param x the x screen coordinate to convert
	//! @return the bin index for the given x coordinate.
	long screenX2DataBin(int x) const;
	//! Convert a bin number to a screen coordinate.
	//! @param x the bin space index; see screenX2DataBin for details
	//! @return the screen space coordinate for the given bin space index
	int  dataBin2ScreenX(long x) const;
	//! Check whether currently a context menu is shown.
	bool isContextMenuVisible() const;
	//! Check whether currently tooltips are enabled.
	bool isTooltipShown() const;
	//! Get the position where the context menu was last shown.
	QPoint contextMenuPos() const;
	//! Set custom x bounds (the interval the x axis covers).
	void setXBounds(double minVal, double maxVal);
	//! Set custom y bounds (the interval the y axis covers).
	void setYBounds(iAPlotData::DataType minVal, iAPlotData::DataType maxVal);
	//! Reset y bounds to the range specified by the current plots.
	void resetYBounds();
	//! Set the caption shown along the x axis.
	void setXCaption(QString const & caption);
	//! Set the caption shown along the y axis.
	void setYCaption(QString const & caption);
	//! Set either linear or logarithmic mapping mode.
	void setYMappingMode(AxisMappingType drawMode);
	//! Set position of x axis caption (Center/Left, Bottom/Top, via Qt::Align... flags).
	void setCaptionPosition(QFlags<Qt::AlignmentFlag>);
	//! Set whether x axis caption should be shown or not.
	void setShowXAxisLabel(bool show);
	//! Add a plot to the chart.
	void addPlot(QSharedPointer<iAPlot> plot);
	//! Remove a plot from the chart.
	void removePlot(QSharedPointer<iAPlot> plot);
	//! Remove all plots from the chart.
	void clearPlots();
	//! Retrieve all plots currently in the chart.
	std::vector< QSharedPointer< iAPlot > > const & plots();
	//! Check whether all plots currently in the chart have discrete
	//! (categorical also counts as discrete for this purpose). If a single
	//! plot is continuous, it will report false.
	bool isDrawnDiscrete() const;
	//! Add an image overlay to the chart.
	void addImageOverlay(QSharedPointer<QImage> imgOverlay);
	//! Remove an image overlay from the chart.
	void removeImageOverlay(QImage * imgOverlay);
	//! Determine how a selection works; see SelectionMode: either disable selection,
	//! or allow selection of single plots.
	void setSelectionMode(SelectionMode mode);
	//! Adds a marker at a specific x position (in data space, see screenX2DataBin
	//! for details) in the given color
	void addXMarker(double xPos, QColor const & color);
	//! Remove the marker at the given x position (in data space, see screenX2DataBin
	//! for details).
	void removeXMarker(double xPos);
	//! Remove all markers.
	void clearMarkers();
	//! Update all bounds such that all current plots are in the visible area of the chart.
	void updateBounds(size_t startPlot = 0);
	//! Update x bounds of the chart such that the x bounds of all current plots are in the visible area of the chart.
	void updateXBounds(size_t startPlot = 0);
	//! Update y bounds of the chart such that the y bounds of all current plots are in the visible area of the chart.
	void updateYBounds(size_t startPlot = 0);
	//! Draws the chart off screen and returns an image of the result.
	QImage drawOffscreen();
	//! Sets the background color of the whole chart.
	void setBackgroundColor(QColor const & color);

public slots:
	//! Reset view (zoom and shift in x and y direction) such that all plots are fully visible.
	void resetView();
	//! Determines whether the x axis is drawn at location of zero in the data, or at the bottom of the chart region
	//! @param enable if true, the x axis is drawn where the value 0 is on the y axis;
	//!     if false, it is drawn at the bottom of the chart region.
	void setDrawXAxisAtZero(bool enable);

signals:
	//! Fires whenever the displayed x axis has changed
	//! (i.e. if it has been shifted, zoomed, or reset).
	void xAxisChanged();
	//! Fires whenever one or more plots are selected.
	//! @param plotIDs the IDs of the selected plots
	void plotsSelected(std::vector<size_t> const & plotIDs);
	//! Fires whenever the user double-clicks on the chart.
	void dblClicked();

protected:
	QString m_xCaption, m_yCaption;
	int m_zoomXPos, m_zoomYPos;
	double m_xZoom, m_yZoom;
	double m_xZoomStart, m_yZoomStart;

	int m_translationX, m_translationY;
	int m_translationStartX, m_translationStartY;
	int m_dragStartPosX, m_dragStartPosY;
	int m_mode;
	//! Main mappers from diagram coordinates to pixel coordinates, for each axis:
	QSharedPointer<iAMapper> m_xMapper, m_yMapper;
	AxisMappingType m_yMappingMode;
	bool m_contextMenuVisible;

	virtual void drawPlots(QPainter& painter);
	virtual void drawAxes(QPainter& painter);
	virtual QString xAxisTickMarkLabel(double value, double stepWidth);

	void zoomAlongY(double value, bool deltaMode);
	void zoomAlongX(double value, int x, bool deltaMode);

	virtual void changeMode(int newMode, QMouseEvent *event);
	virtual void showDataTooltip(QHelpEvent *event);
	virtual void drawBackground(QPainter &painter);

	//! @{ Overriden Qt events.
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void paintGL() override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	bool event(QEvent *event) override;
	//! @}

private slots:
	void showTooltip(bool toggled);
	void exportData();

private:
	virtual void addContextMenuEntries(QMenu* contextMenu);
	void createMappers();
	void drawAll(QPainter& painter);
	void drawImageOverlays(QPainter &painter);
	virtual void drawAfterPlots(QPainter& painter);
	virtual void drawXAxis(QPainter &painter);
	virtual void drawYAxis(QPainter &painter);
	double visibleXStart() const;
	double visibleXEnd() const;

	std::vector< QSharedPointer< iAPlot > >	m_plots;
	QList< QSharedPointer< QImage > > m_overlays;
	QMenu* m_contextMenu;
	QPoint m_contextPos;
	bool m_showTooltip;
	bool m_showXAxisLabel;
	int  m_fontHeight;
	int  m_yMaxTickLabelWidth;
	bool m_customXBounds, m_customYBounds;
	double m_xBounds[2], m_yBounds[2], m_xTickBounds[2];
	QFlags<Qt::AlignmentFlag> m_captionPosition;
	SelectionMode m_selectionMode;
	QRubberBand* m_selectionBand;
	QPoint m_selectionOrigin;
	std::vector<size_t> m_selectedPlots;
	QMap<double, QColor> m_xMarker;
	size_t m_maxXAxisSteps;
	bool m_drawXAxisAtZero;
	QColor m_bgColor;
};
