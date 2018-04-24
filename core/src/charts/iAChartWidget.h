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
	int    XShift() const { return translationX; }
	int    YShift() const { return translationY; }
	virtual int BottomMargin() const;
	virtual int LeftMargin() const;
	int ActiveWidth()  const;
	int ActiveHeight() const;
	int Height() const;
	iAPlotData::DataType GetMaxYDataValue() const;
	QSharedPointer<iAMapper> const YMapper() const;
	virtual iAPlotData::DataType const * YBounds() const;
	virtual double const * XBounds() const;
	virtual size_t MaxXAxisSteps() const;
	bool CategoricalAxis() const;
	double XRange() const;
	double MaxXZoom() const;
	int diagram2PaintX(double x) const;
	long screenX2DataBin(int x) const;
	int  dataBin2ScreenX(long x) const;
	bool IsContextMenuVisible() const;
	bool IsTooltipShown() const;
	QPoint ContextMenuPos() const;

	void SetXBounds(double minVal, double maxVal);
	void SetYBounds(iAPlotData::DataType minVal, iAPlotData::DataType maxVal);
	void ResetYBounds();
	void SetXCaption(QString const & caption);
	void SetYMappingMode(AxisMappingType drawMode);
	void SetCaptionPosition(QFlags<Qt::AlignmentFlag>);
	void SetShowXAxisLabel(bool show);

	void AddPlot(QSharedPointer<iAPlot> plot);
	void RemovePlot(QSharedPointer<iAPlot> plot);
	QVector< QSharedPointer< iAPlot > > const & Plots();
	bool IsDrawnDiscrete() const;
	void AddImageOverlay(QSharedPointer<QImage> imgOverlay);
	void RemoveImageOverlay(QImage * imgOverlay);

	virtual void redraw();
public slots:
	void resetView();
signals:
	void XAxisChanged();
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
	QImage m_drawBuffer;

	virtual void DrawPlots(QPainter& painter);
	virtual void DrawAxes(QPainter& painter);
	virtual QString GetXAxisTickMarkLabel(double value, int placesBeforeComma, int requiredPlacesAfterComma);

	void zoomAlongY(double value, bool deltaMode);
	void zoomAlongX(double value, int x, bool deltaMode);

	virtual void setNewSize();
	virtual void changeMode(int newMode, QMouseEvent *event);
	virtual void showDataTooltip(QMouseEvent *event);
	virtual void DrawEverything();
	virtual void DrawBackground(QPainter &painter);

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
	void ExportData();
private:
	virtual void AddContextMenuEntries(QMenu* contextMenu);
	void CreateYConverter();
	void DrawImageOverlays(QPainter &painter);
	virtual void DrawAfterPlots(QPainter& painter);
	virtual void DrawXAxis(QPainter &painter);
	virtual void DrawYAxis(QPainter &painter);
	void UpdateBounds();
	void UpdateXBounds();
	void UpdateYBounds();

	QVector< QSharedPointer< iAPlot > >	m_plots;
	QList< QSharedPointer< QImage > > m_overlays;
	QMenu* m_contextMenu;
	QPoint m_contextPos;
	bool m_showTooltip;
	bool m_showXAxisLabel;
	int  m_fontHeight;
	int  m_yMaxTickLabelWidth;
	bool m_draw;
	bool m_customXBounds, m_customYBounds;
	double m_xBounds[2], m_yBounds[2];
	QFlags<Qt::AlignmentFlag> m_captionPosition;
};
