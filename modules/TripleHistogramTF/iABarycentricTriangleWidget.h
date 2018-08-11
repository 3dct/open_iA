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

#define _USE_MATH_DEFINES // necessary to use M_PI (with math.height)
#include <math.h>

// Constants
// TODO: is this really the way to declare constants?
static const qreal RAD60 = M_PI / 3.0;
static const qreal SIN60 = sin(RAD60);
static const qreal ONE_DIV_SIN60 = 1.0 / SIN60;
static const qreal COS60 = 0.5;
static const qreal ONE_DIV_THREE = 1.0 / 3.0;

static const int CONTROL_POINT_RADIUS = 10;
static const int MODALITY_LABEL_MARGIN = 10;
static const int MODALITY_LABEL_MARGIN_TIMES_TWO = MODALITY_LABEL_MARGIN * 2;

static const QString MODALITY_LABEL_1_DEFAULT = "A";
static const QString MODALITY_LABEL_2_DEFAULT = "B";
static const QString MODALITY_LABEL_3_DEFAULT = "C";

class iABarycentricTriangleWidget : public QOpenGLWidget, public IBorderWidget
{
	Q_OBJECT

public:
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

public slots:
	//void mousePress(QMouseEvent*);
	//void mouseMove(QMouseEvent*);
	//void mouseWheel(QWheelEvent*);

signals:
	void weightChanged(BCoord bCoord);

protected:
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

private:
	BarycentricTriangle m_triangle;
	QPoint m_controlPoint;
	QPoint m_controlPointOld;
	BCoord m_controlPointBCoord;

	QString m_modalityLabel1 = MODALITY_LABEL_1_DEFAULT;
	QString m_modalityLabel2 = MODALITY_LABEL_2_DEFAULT;
	QString m_modalityLabel3 = MODALITY_LABEL_3_DEFAULT;

	QFont m_modalityLabelFont;
	QPoint m_modalityLabel1Pos;
	QPoint m_modalityLabel2Pos;
	QPoint m_modalityLabel3Pos;

	QPainterPath m_trianglePainterPath;
	QPen m_triangleBorderPen;// = QPen(); // TODO: could be constant...?
	QBrush m_triangleFillBrush;

	QPainterPath m_controlPointBorderPainterPath;
	QPen m_controlPointBorderPen;
	QPainterPath m_controlPointCrossPainterPath;
	QPen m_controlPointCrossPen;

	void initializeControlPointPaths();
	void updateControlPointPosition(QPoint newPos);
	void updateControlPointPosition();
	void moveControlPointTo(QPoint newPos);

	void recalculatePositions(int w, int h);

	void paintTriangleBorder(QPainter &p);
	void paintTriangleFill(QPainter &p);
	void paintControlPoint(QPainter &p);
	void paintModalityLabels(QPainter &p);

	bool isTooWide(int width, int height);
	bool isTooTall(int width, int height);

};