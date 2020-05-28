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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	iABarycentricTriangleWidget(QWidget* parent = nullptr, Qt::WindowFlags f = 0);
#else
	iABarycentricTriangleWidget(QWidget* parent = nullptr, Qt::WindowFlags f = QFlags<Qt::WindowType>());
#endif

	int getWidthForHeight(int height);
	int getHeightForWidth(int width);

	void recalculatePositions() { recalculatePositions(width(), height()); }
	void recalculatePositions(int width, int height, iABarycentricTriangle triange);

	iABCoord getWeight();
	void setWeight(iABCoord newWeight);

	void setTriangleRenderer(iABarycentricContextRenderer *triangleRenderer);
	void setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3);
	void updateModalityNames(QString const name[3]);

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
	void paintEvent(QPaintEvent* event);
	void resizeEvent(QResizeEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

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

	void updateControlPointCoordinates(iABCoord bc) {
		int a = bc[0] * 100;
		int b = bc[1] * 100;
		int c = 100 - a - b;
		updateControlPointCoordinates(bc, a, b, c);
	}

	void updateControlPointCoordinates(iABCoord bCoord, int a, int b, int c) {
		updateControlPoint(bCoord, m_triangle.getCartesianCoordinates(bCoord), a, b, c);
	}

	void updateControlPointPosition(QPoint newPos) {
		auto bc = m_triangle.getBarycentricCoordinates(newPos.x(), newPos.y());
		int a = bc[0] * 100;
		int b = bc[1] * 100;
		int c = 100 - a - b;
		updateControlPoint(bc, newPos, a, b, c);
	}

	void updateControlPointPosition() {
		updateControlPointCoordinates(m_controlPointBCoord);
	}

	void recalculatePositions(int w, int h);
	void recalculatePositions(int w, int h, bool changeTriangle);

	bool isTooWide(int width, int height);
	bool isTooTall(int width, int height);

};