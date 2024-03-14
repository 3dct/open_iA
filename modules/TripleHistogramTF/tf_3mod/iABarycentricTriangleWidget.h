// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iABarycentricTriangle.h"
#include "iABCoord.h"

#include <vtkSmartPointer.h>

#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QWidget>

class iABarycentricContextRenderer;

class vtkImageData;

class QSpinBox;

class iABarycentricTriangleWidget : public QWidget
{
	Q_OBJECT

public:
	iABarycentricTriangleWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	int getWidthForHeight(int height);
	int getHeightForWidth(int width);

	void recalculatePositions()
	{
		recalculatePositions(width(), height());
	}
	void recalculatePositions(int width, int height, iABarycentricTriangle triange);

	iABCoord getWeight();
	void setWeight(iABCoord newWeight);

	void setTriangleRenderer(iABarycentricContextRenderer *triangleRenderer);
	void setData(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3);
	void updateDataSetNames(std::array<QString, 3> names);

	iABarycentricTriangle getTriangle() { return m_triangle; }

	void paintTriangleFill(QPainter &p);
	void paintTriangleBorder(QPainter &p);
	void paintContext(QPainter &p);
	void paintControlPoint(QPainter &p);

signals:
	void weightsChanged(iABCoord bCoord);

private slots:
	void onHeatmapReady();

	void onSpinBoxValueChanged_1(int newValue);
	void onSpinBoxValueChanged_2(int newValue);
	void onSpinBoxValueChanged_3(int newValue);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	iABarycentricTriangle m_triangle;
	QPoint m_controlPoint;
	QPoint m_controlPointOld;
	iABCoord m_controlPointBCoord;

	QSpinBox *m_spinBoxes[3];

	QPainterPath m_trianglePainterPath;
	QBrush m_triangleFillBrush;
	QPen m_triangleBorderPen;

	QPainterPath m_controlPointBorderPainterPath;
	QPen m_controlPointBorderPen;
	QPainterPath m_controlPointCrossPainterPath;
	QPen m_controlPointCrossPen;

	iABarycentricContextRenderer *m_triangleRenderer = nullptr;

	bool m_dragging = false;

	void initializeControlPointPaths();
	void updateControlPoint(iABCoord bCoord, QPoint newPos, int a, int b, int c);
	void moveControlPointTo(QPoint newPos);

	void updateControlPointCoordinates(iABCoord bc)
	{
		int a = bc[0] * 100;
		int b = bc[1] * 100;
		int c = 100 - a - b;
		updateControlPointCoordinates(bc, a, b, c);
	}

	void updateControlPointCoordinates(iABCoord bCoord, int a, int b, int c)
{
		updateControlPoint(bCoord, m_triangle.getCartesianCoordinates(bCoord), a, b, c);
	}

	void updateControlPointPosition(QPoint newPos)
	{
		auto bc = m_triangle.getBarycentricCoordinates(newPos.x(), newPos.y());
		int a = bc[0] * 100;
		int b = bc[1] * 100;
		int c = 100 - a - b;
		updateControlPoint(bc, newPos, a, b, c);
	}

	void updateControlPointPosition()
	{
		updateControlPointCoordinates(m_controlPointBCoord);
	}

	void recalculatePositions(int w, int h);
	void recalculatePositions(int w, int h, bool changeTriangle);

	bool isTooWide(int width, int height);
	bool isTooTall(int width, int height);

};
