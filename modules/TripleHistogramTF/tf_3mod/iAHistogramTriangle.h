// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	enum MouseEventType
	{
		PRESS, MOVE, RELEASE
	};
	enum WidgetType
	{
		NONE, HISTOGRAM, SLICER, TRIANGLE
	};

public:
	iAHistogramTriangle(iATripleModalityWidget* tripleModalityWidget);

	void initialize(std::array<QString, 3> names) override;
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
	void calculatePositions()
	{
		calculatePositions(size().width(), size().height());
	}
	void calculatePositions(int w, int h);

	void forwardMouseEvent(QMouseEvent *event, MouseEventType type);
	void forwardWheelEvent(QWheelEvent *event);
	void forwardContextMenuEvent(QContextMenuEvent *event);
	iAChartWithFunctionsWidget* onHistogram(QPoint p, QPoint &transformed);
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
