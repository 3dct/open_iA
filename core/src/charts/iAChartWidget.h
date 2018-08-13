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
#pragma once

#include "open_iA_Core_export.h"

#include "iAPlotData.h"

#define WIN32_LEAN_AND_MEAN		// apparently QGLWidget might include windows.h...
#define NOMINMAX
#include <QGLWidget>
#include <QImage>

class iAPlot;
class iAMapper;

class QMenu;

class open_iA_Core_API iAChartWidget : public QGLWidget
{
	Q_OBJECT
public:
	enum Mode { NO_MODE, MOVE_VIEW_MODE, X_ZOOM_MODE, Y_ZOOM_MODE };
	enum AxisMappingType { Linear, Logarithmic };
	iAChartWidget(QWidget* parent, QString const & xLabel, QString const & yLabel);
	virtual ~iAChartWidget();

	double XZoom()  const { return xZoom;        }
	double YZoom()  const { return yZoom;        }
	int    xShift() const { return translationX; }
	int    yShift() const { return translationY; }
	virtual int bottomMargin() const;
	virtual int leftMargin() const;
	int activeWidth()  const;
	int activeHeight() const;
	int Height() const;
	iAPlotData::DataType getMaxYDataValue() const;
	QSharedPointer<iAMapper> const yMapper() const;
	virtual iAPlotData::DataType const * yBounds() const;
	virtual double const * xBounds() const;
	virtual size_t maxXAxisSteps() const;
	bool categoricalAxis() const;
	double xRange() const;
	double maxXZoom() const;
	int diagram2PaintX(double x) const;
	long screenX2DataBin(int x) const;
	int  dataBin2ScreenX(long x) const;
	bool isContextMenuVisible() const;
	bool isTooltipShown() const;
	QPoint contextMenuPos() const;

	void setXBounds(double minVal, double maxVal);
	void setYBounds(iAPlotData::DataType minVal, iAPlotData::DataType maxVal);
	void resetYBounds();
	void setXCaption(QString const & caption);
	void setYMappingMode(AxisMappingType drawMode);
	void setCaptionPosition(QFlags<Qt::AlignmentFlag>);
	void setShowXAxisLabel(bool show);

	void addPlot(QSharedPointer<iAPlot> plot);
	void removePlot(QSharedPointer<iAPlot> plot);
	QVector< QSharedPointer< iAPlot > > const & plots();
	bool isDrawnDiscrete() const;
	void addImageOverlay(QSharedPointer<QImage> imgOverlay);
	void removeImageOverlay(QImage * imgOverlay);

public slots:
	void resetView();
signals:
	void xAxisChanged();
protected:
	QString xCaption, yCaption;
	int zoomX;
	int zoomY;
	double yZoom;
	double yZoomStart;
	double xZoom;
	double xZoomStart;

	int translationX;
	int translationY;
	int translationStartX;
	int translationStartY;
	int dragStartPosX;
	int dragStartPosY;

	int width, height;
	int mode;
	QSharedPointer<iAMapper> m_yConverter;
	AxisMappingType m_yMappingMode;
	bool m_contextMenuVisible;

	virtual void drawPlots(QPainter& painter);
	virtual void drawAxes(QPainter& painter);
	virtual QString getXAxisTickMarkLabel(double value, double stepWidth);

	void zoomAlongY(double value, bool deltaMode);
	void zoomAlongX(double value, int x, bool deltaMode);

	virtual void setNewSize();
	virtual void changeMode(int newMode, QMouseEvent *event);
	virtual void showDataTooltip(QMouseEvent *event);
	virtual void drawEverything(QPainter &painter);
	virtual void drawBackground(QPainter &painter);

	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
private slots:
	void showTooltip(bool toggled);
	void exportData();
private:
	virtual void addContextMenuEntries(QMenu* contextMenu);
	void createYConverter();
	void drawImageOverlays(QPainter &painter);
	virtual void drawAfterPlots(QPainter& painter);
	virtual void drawXAxis(QPainter &painter);
	virtual void drawYAxis(QPainter &painter);
	void updateBounds();
	void updateXBounds();
	void updateYBounds();

	QVector< QSharedPointer< iAPlot > >	m_plots;
	QList< QSharedPointer< QImage > > m_overlays;
	QMenu* m_contextMenu;
	QPoint m_contextPos;
	bool m_showTooltip;
	bool m_showXAxisLabel;
	int  m_fontHeight;
	int  m_yMaxTickLabelWidth;
	bool m_customXBounds, m_customYBounds;
	double m_xBounds[2], m_yBounds[2];
	QFlags<Qt::AlignmentFlag> m_captionPosition;
};
