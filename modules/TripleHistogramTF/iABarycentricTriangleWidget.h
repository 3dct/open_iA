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

#include <QOpenGLWidget>
#include <QWidget>
#include <QPainterPath>
#include <QPen>
#include <QPoint>

#include "RightBorderLayout.h"
#include "BarycentricTriangle.h"
#include "BCoord.h"
class iATriangleRenderer;

#include "vtkSmartPointer.h"
#include "vtkImageData.h"

// Constants (more in the cpp file!)
static const QString MODALITY_LABEL_1_DEFAULT = "A";
static const QString MODALITY_LABEL_2_DEFAULT = "B";
static const QString MODALITY_LABEL_3_DEFAULT = "C";

static const QColor BACKGROUND_DEFAULT = QColor(242, 242, 242, 255);

class iABarycentricTriangleWidget : public QOpenGLWidget, public IBorderWidget
{
	Q_OBJECT

public:
	iABarycentricTriangleWidget(QWidget* parent, QColor backgroundColor, Qt::WindowFlags f = 0);
	iABarycentricTriangleWidget(QWidget* parent, Qt::WindowFlags f = 0);
	
	~iABarycentricTriangleWidget();

	bool hasWidthForHeight() override;
	int getWidthForHeight(int height) override;
	bool hasHeightForWidth() override;
	int getHeightForWidth(int width) override;
	QWidget* widget() override;

	int getWidthForCurrentHeight();
	int getHeightForCurrentWidth();

	void recalculatePositions() { recalculatePositions(width(), height()); }
	void setFont(QFont font);
	void setModality1label(QString label);
	void setModality2label(QString label);
	void setModality3label(QString label);

	BCoord getControlPointCoordinates();

	void setTriangleRenderer(iATriangleRenderer *triangleRenderer);
	void setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3);

	void setBackgroundColor(QColor color);

public slots:

signals:
	void weightChanged(BCoord bCoord);

protected:
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	QColor m_backgroundColor = BACKGROUND_DEFAULT;
	BarycentricTriangle m_triangle;
	QPoint m_controlPoint;
	QPoint m_controlPointOld;
	BCoord m_controlPointBCoord;

	QFont m_modalityLabelFont;
	QString m_modalityLabel1 = MODALITY_LABEL_1_DEFAULT;
	QString m_modalityLabel2 = MODALITY_LABEL_2_DEFAULT;
	QString m_modalityLabel3 = MODALITY_LABEL_3_DEFAULT;
	QPoint m_modalityLabel1Pos;
	QPoint m_modalityLabel2Pos;
	QPoint m_modalityLabel3Pos;
	QRect m_modalityLabelRect[3];
	QPen m_modalityLabelHighlightPen;
	int m_modalityHighlightedIndex = -1; // -1 for none (or any value < 0)
	bool interactWithModalityLabel(QPoint p, bool press);

	QFont m_modalityWeightFont;
	QString m_modalityWeight1;
	QString m_modalityWeight2;
	QString m_modalityWeight3;
	QPoint m_modalityWeight1Pos;
	QPoint m_modalityWeight2Pos;
	QPoint m_modalityWeight3Pos;
	void updateModalityWeightLabels(BCoord bCoord);

	QPainterPath m_trianglePainterPath;
	QBrush m_triangleFillBrush;
	QPen m_triangleBorderPen;

	QPainterPath m_controlPointBorderPainterPath;
	QPen m_controlPointBorderPen;
	QPainterPath m_controlPointCrossPainterPath;
	QPen m_controlPointCrossPen;

	iATriangleRenderer *m_triangleRenderer = nullptr;

	bool m_dragging = false;

	void initializeControlPointPaths();
	void updateControlPointCoordinates(BCoord bCoord);
	void updateControlPointPosition(QPoint newPos);
	void updateControlPointPosition();
	void moveControlPointTo(QPoint newPos);

	void recalculatePositions(int w, int h);

	void paintTriangleFill(QPainter &p);
	void paintTriangleBorder(QPainter &p);
	void paintHelper(QPainter &p);
	void paintControlPoint(QPainter &p);
	void paintModalityLabels(QPainter &p);

	bool isTooWide(int width, int height);
	bool isTooTall(int width, int height);

};