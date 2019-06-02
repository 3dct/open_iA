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
	virtual ~iAChartWidget();
	double xZoom()  const { return m_xZoom;        }
	double yZoom()  const { return m_yZoom;        }
	int    xShift() const { return m_translationX; }
	int    yShift() const { return m_translationY; }
	virtual int bottomMargin() const;
	virtual int leftMargin() const;
	virtual int activeWidth()  const;
	virtual int activeHeight() const;
	iAPlotData::DataType minYDataValue(size_t startPlot = 0) const;
	iAPlotData::DataType maxYDataValue(size_t startPlot = 0) const;
	iAMapper const & xMapper() const;
	iAMapper const & yMapper() const;
	virtual iAPlotData::DataType const * yBounds() const;
	virtual double const * xBounds() const;
	bool categoricalAxis() const;
	double xRange() const;
	double maxXZoom() const;
	long screenX2DataBin(int x) const;
	int  dataBin2ScreenX(long x) const;
	bool isContextMenuVisible() const;
	bool isTooltipShown() const;
	QPoint contextMenuPos() const;
	void setXBounds(double minVal, double maxVal);
	void setYBounds(iAPlotData::DataType minVal, iAPlotData::DataType maxVal);
	void resetYBounds();
	void setXCaption(QString const & caption);
	void setYCaption(QString const & caption);
	void setYMappingMode(AxisMappingType drawMode);
	void setCaptionPosition(QFlags<Qt::AlignmentFlag>);
	void setShowXAxisLabel(bool show);
	void addPlot(QSharedPointer<iAPlot> plot);
	void removePlot(QSharedPointer<iAPlot> plot);
	void clearPlots();
	std::vector< QSharedPointer< iAPlot > > const & plots();
	bool isDrawnDiscrete() const;
	void addImageOverlay(QSharedPointer<QImage> imgOverlay);
	void removeImageOverlay(QImage * imgOverlay);
	void setSelectionMode(SelectionMode mode);
	void addXMarker(double xPos, QColor const & color);
	void removeXMarker(double xPos);
	void clearMarkers();
	void updateBounds(size_t startPlot = 0);
	void updateXBounds(size_t startPlot = 0);
	void updateYBounds(size_t startPlot = 0);
	QImage drawOffscreen();
	void setBackgroundColor(QColor const & color);

public slots:
	void resetView();
	void setDrawXAxisAtZero(bool enable);

signals:
	void xAxisChanged();
	void plotsSelected(std::vector<size_t> const & plotIDs);
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
