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

#include "RightBorderLayout.h"
#include "BarycentricTriangle.h"
#include "BCoord.h"

#include <vtkSmartPointer.h>

#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QWidget>

class iATriangleRenderer;

class vtkImageData;

// Constants (more in the cpp file!)
static const QString MODALITY_LABEL_1_DEFAULT = "A";
static const QString MODALITY_LABEL_2_DEFAULT = "B";
static const QString MODALITY_LABEL_3_DEFAULT = "C";

static const QColor BACKGROUND_DEFAULT = QColor(242, 242, 242, 255);

class iABarycentricTriangleWidget : public QWidget, public IBorderWidget
{
	Q_OBJECT

public:
	iABarycentricTriangleWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	
	~iABarycentricTriangleWidget();

	bool hasWidthForHeight() override;
	int getWidthForHeight(int height) override;
	bool hasHeightForWidth() override;
	int getHeightForWidth(int width) override;
	QWidget* widget() override;

	int getWidthForCurrentHeight();
	int getHeightForCurrentWidth();

	void recalculatePositions() { recalculatePositions(width(), height()); }
	void recalculatePositions(int width, int height, BarycentricTriangle triange);
	void setFont(QFont font);
	void setModality1label(QString label);
	void setModality2label(QString label);
	void setModality3label(QString label);
	void setModalityLabelPosition(QPoint position, int modalityIndex);
	void setModalityWeightPosition(QPoint position, int modalityIndex);
	QRect getModalityLabelRect(int modalityIndex);
	QRect getModalityWeightRect(int modalityIndex);

	BCoord getWeight();
	void setWeight(BCoord newWeight);

	void setTriangleRenderer(iATriangleRenderer *triangleRenderer);
	void setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3);

	BarycentricTriangle getTriangle() { return m_triangle; }

	void paintTriangleFill(QPainter &p);
	void paintTriangleBorder(QPainter &p);
	void paintContext(QPainter &p);
	void paintControlPoint(QPainter &p);
	void paintModalityLabels(QPainter &p);

public slots:

signals:
	void weightChanged(BCoord bCoord);

protected:
	void paintEvent(QPaintEvent* event);
	void resizeEvent(QResizeEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

private:
	BarycentricTriangle m_triangle;
	QPoint m_controlPoint;
	QPoint m_controlPointOld;
	BCoord m_controlPointBCoord;

	QFont m_modalityLabelFont;
	QString m_modalityLabel1 = MODALITY_LABEL_1_DEFAULT;
	QString m_modalityLabel2 = MODALITY_LABEL_2_DEFAULT;
	QString m_modalityLabel3 = MODALITY_LABEL_3_DEFAULT;
	QPoint m_modalityLabelPos[3];
	QRect m_modalityLabelRect[3];
	QPen m_modalityLabelHighlightPen;
	int m_modalityHighlightedIndex = -1; // -1 for none (or any value < 0)
	bool interactWithModalityLabel(QPoint p, bool press);

	QFont m_modalityWeightFont;
	QString m_modalityWeight1;
	QString m_modalityWeight2;
	QString m_modalityWeight3;
	QPoint m_modalityWeightPos[3];
	QRect m_modalityWeightRect[3];
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
	void recalculatePositions(int w, int h, bool changeTriangle);

	bool isTooWide(int width, int height);
	bool isTooTall(int width, int height);

	void clearGL();

};