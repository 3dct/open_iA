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

#include "iAHistogramAbstract.h"

#include "iABarycentricTriangle.h"

#include <QPainterPath>
#include <QPen>
#include <QRect>

class iASlicer;
class iAChartWithFunctionsWidget;

class QPoint;
class QMouseEvent;
class QWheelEvent;

class iAHistogramTriangle : public iAHistogramAbstract
{
	Q_OBJECT

private:
	enum MouseEventType {
		PRESS, MOVE, RELEASE
	};
	enum WidgetType {
		NONE, HISTOGRAM, SLICER, TRIANGLE
	};

public:
	iAHistogramTriangle(iATripleModalityWidget* tripleModalityWidget);

	void initialize(QString const names[3]) override;
	bool isSlicerInteractionEnabled() override { return true; }

	void paintHistograms(QPainter &p);
	void paintSlicers(QPainter& p);
	void paintLabels(QPainter& p);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

	void mousePressEvent(QMouseEvent *event) override { forwardMouseEvent(event, PRESS); }
	void mouseMoveEvent(QMouseEvent *event) override { forwardMouseEvent(event, MOVE); }
	void mouseReleaseEvent(QMouseEvent *event) override { forwardMouseEvent(event, RELEASE); }
	void wheelEvent(QWheelEvent *event) override { forwardWheelEvent(event); }
	void contextMenuEvent(QContextMenuEvent *event) override { forwardContextMenuEvent(event); }

private slots:
	void updateSlicers();
	void updateHistograms();

	// Debug
	void glresized();

private:
	void calculatePositions() { calculatePositions(size().width(), size().height()); }
	void calculatePositions(int w, int h);

	void forwardMouseEvent(QMouseEvent *event, MouseEventType type);
	void forwardWheelEvent(QWheelEvent *event);
	void forwardContextMenuEvent(QContextMenuEvent *event);
	QSharedPointer<iAChartWithFunctionsWidget> onHistogram(QPoint p, QPoint &transformed);
	bool onTriangle(QPoint p);
	iASlicer* onSlicer(QPoint p, QPoint &transformed);

	QWidget* m_draggedWidget = nullptr;
	WidgetType m_draggedType = NONE;
	int m_lastIndex = -1;

	iATripleModalityWidget* m_tmw;

	QPainterPath m_clipPath;
	QPen m_clipPathPen;

	QTransform m_transformHistograms[3]; // left, right, bottom (respectively)
	QRect m_histogramsRect;

	QTransform m_transformSlicers[3]; // left(A), top(B), right(C) (respectively)
	iABarycentricTriangle m_slicerTriangles[3];
	QPainterPath m_slicerClipPaths[3];
	QBrush m_slicerBackgroundBrush;

	int m_triangleBigWidth;

	// Buffer
	QImage m_buffer;

	// Painting rectangles
	QRect m_rControls;
	//QRect m_rHistogram[3];
	//QRect m_rSlicer[3];
	//QRect m_rTriangle;
	QRect m_rLabels[3];

	// Painting flags
	bool m_fClear;
	bool m_fRenderHistogram[3];
	bool m_fRenderSlicer[3];
	bool m_fRenderTriangle;
};