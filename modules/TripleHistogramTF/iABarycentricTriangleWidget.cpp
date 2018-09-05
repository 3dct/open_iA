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

#include "iABarycentricTriangleWidget.h"
#include "iATriangleRenderer.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QString>
#define _USE_MATH_DEFINES // necessary to use M_PI (with math.height)
#include <math.h>

// TODO: really necessary? (just to get the font()!)
#include <QApplication>

// Debug
#include <QDebug>

// Constants (more in the header file!)
static const qreal RAD60 = M_PI / 3.0;
static const qreal SIN60 = sin(RAD60);
static const qreal ONE_DIV_SIN60 = 1.0 / SIN60;
static const qreal COS60 = 0.5;
static const qreal ONE_DIV_THREE = 1.0 / 3.0;

static const int CONTROL_POINT_RADIUS = 10;
static const int MODALITY_LABEL_MARGIN = 10;
static const int MODALITY_LABEL_MARGIN_TIMES_TWO = MODALITY_LABEL_MARGIN * 2;
//static const int MODALITY_LABEL_HIGHLIGHT_PADDING = 5;
//static const int MODALITY_LABEL_HIGHLIGHT_PADDING_TIMES_TWO = MODALITY_LABEL_HIGHLIGHT_PADDING * 2;

static const char* WEIGHT_FORMAT = "%.0f%%"; //"%.2f%;

iABarycentricTriangleWidget::iABarycentricTriangleWidget(QColor backgroundColor, QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	iABarycentricTriangleWidget(parent, f)
{
	setBackgroundColor(backgroundColor);
}

iABarycentricTriangleWidget::iABarycentricTriangleWidget(QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	QOpenGLWidget(parent, f)
{
	// Font variables are values (not pointers)
	m_modalityLabelFont = QApplication::font();
	m_modalityWeightFont = m_modalityLabelFont;

	m_modalityLabelFont.setPointSize(16);

	m_modalityWeightFont.setPointSize(12);

	m_controlPointBorderPen.setWidth(3);
	m_controlPointBorderPen.setColor(Qt::black);

	m_controlPointCrossPen.setWidth(2);
	m_controlPointCrossPen.setColor(Qt::black);

	m_triangleFillBrush.setColor(Qt::white);

	m_triangleBorderPen.setWidth(5);
	m_triangleBorderPen.setColor(Qt::black);

	m_modalityLabelHighlightPen.setWidth(2);
	m_modalityLabelHighlightPen.setColor(Qt::black);

	setMouseTracking(true); // to enable mouse move events without the mouse button needing to be pressed

	initializeControlPointPaths();
}

void iABarycentricTriangleWidget::initializeControlPointPaths()
{
	m_controlPointBorderPainterPath = QPainterPath();
	m_controlPointBorderPainterPath.addEllipse(m_controlPoint, CONTROL_POINT_RADIUS, CONTROL_POINT_RADIUS);

	m_controlPointCrossPainterPath = QPainterPath();
	m_controlPointCrossPainterPath.moveTo(
		m_controlPoint.x() - CONTROL_POINT_RADIUS,
		m_controlPoint.y()
	);
	m_controlPointCrossPainterPath.lineTo(
		m_controlPoint.x() + CONTROL_POINT_RADIUS,
		m_controlPoint.y()
	);
	m_controlPointCrossPainterPath.moveTo(
		m_controlPoint.x(),
		m_controlPoint.y() - CONTROL_POINT_RADIUS
	);
	m_controlPointCrossPainterPath.lineTo(
		m_controlPoint.x(),
		m_controlPoint.y() + CONTROL_POINT_RADIUS
	);
}

iABarycentricTriangleWidget::~iABarycentricTriangleWidget()
{
}

void iABarycentricTriangleWidget::mousePressEvent(QMouseEvent *event)
{
	if (!interactWithModalityLabel(event->pos(), true)) {
		updateControlPointPosition(event->pos());
		m_dragging = true;
	}
}

void iABarycentricTriangleWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_dragging) {
		updateControlPointPosition(event->pos());
	} else {
		interactWithModalityLabel(event->pos(), false);
	}
}

void iABarycentricTriangleWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_dragging = false;
}

void iABarycentricTriangleWidget::recalculatePositions(int width, int height)
{
	recalculatePositions(width, height, true);
}

void iABarycentricTriangleWidget::recalculatePositions(int width, int height, BarycentricTriangle triangle)
{
	m_triangle = triangle;
	recalculatePositions(width, height, false);
}

void iABarycentricTriangleWidget::recalculatePositions(int width, int height, bool changeTriangle)
{
	QFontMetrics metrics = QFontMetrics(m_modalityLabelFont);
	int modalityLabelHeight = metrics.height();
	
	int triangleSpacingLeft = MODALITY_LABEL_MARGIN; // LEFT margin of BOTTOM-LEFT modality
	int triangleSpacingTop = modalityLabelHeight + MODALITY_LABEL_MARGIN_TIMES_TWO; // complete height of TOP modality
	int triangleSpacingRight = MODALITY_LABEL_MARGIN; // RIGHT margin of BOTTOM-RIGHT modality
	int triangleSpacingBottom = triangleSpacingTop; // complete height of BOTTOM modality

	int aw = width - triangleSpacingLeft - triangleSpacingRight; // available width (for the triangle)
	int ah = height - triangleSpacingTop - triangleSpacingBottom; // available height (for the triangle)
	int tw, th; // triangle's width and height
	if (isTooWide(aw, ah))
	{
		tw = aw;
		th = getHeightForWidth(aw);
	}
	else {
		tw = getWidthForHeight(ah);
		th = ah;
	}

	// triangle rect
	int top, left, right, bottom, centerX;
	left = triangleSpacingLeft + ((aw - tw) / 2);
	top = triangleSpacingTop + ((ah - th) / 2);
	right = left + tw;
	bottom = top + th;
	centerX = left + (tw / 2);

	if (changeTriangle) {
		m_triangle.setXa(left);
		m_triangle.setYa(bottom);
		m_triangle.setXb(centerX);
		m_triangle.setYb(top);
		m_triangle.setXc(right);
		m_triangle.setYc(bottom);
	}

	m_trianglePainterPath = QPainterPath();
	m_trianglePainterPath.moveTo(m_triangle.getXa(), m_triangle.getYa());
	m_trianglePainterPath.lineTo(m_triangle.getXb(), m_triangle.getYb());
	m_trianglePainterPath.lineTo(m_triangle.getXc(), m_triangle.getYc());
	m_trianglePainterPath.lineTo(m_triangle.getXa(), m_triangle.getYa());

	// LABELS PLACEMENT {
	int modalityLabel1width = metrics.width(m_modalityLabel1);
	int modalityLabel2width = metrics.width(m_modalityLabel2);
	int modalityLabel3width = metrics.width(m_modalityLabel3);

	// TODO: position the edge labels correctly even with externally set triangles (not equilateral)
	int modalityLabel1Left = m_triangle.getXa() + MODALITY_LABEL_MARGIN;
	int modalityLabel2Left = m_triangle.getXb() - (modalityLabel2width / 2);
	int modalityLabel3Left = m_triangle.getXc() - modalityLabel3width - MODALITY_LABEL_MARGIN;

	int modalityLabel1_3Top = m_triangle.getYa() + MODALITY_LABEL_MARGIN;
	int modalityLabel1_3Bottom = modalityLabel1_3Top + modalityLabelHeight;
	int modalityLabel2Bottom = m_triangle.getYb() - MODALITY_LABEL_MARGIN;

	m_modalityLabel1Pos = QPoint(modalityLabel1Left, modalityLabel1_3Bottom); // bottom left
	m_modalityLabel2Pos = QPoint(modalityLabel2Left, modalityLabel2Bottom); // top centerX
	m_modalityLabel3Pos = QPoint(modalityLabel3Left, modalityLabel1_3Bottom); // bottom right

	m_modalityLabelRect[0] = QRect(modalityLabel1Left, modalityLabel1_3Top, modalityLabel1width, modalityLabelHeight);
	m_modalityLabelRect[1] = QRect(modalityLabel2Left, modalityLabel2Bottom - modalityLabelHeight, modalityLabel2width, modalityLabelHeight);
	m_modalityLabelRect[2] = QRect(modalityLabel3Left, modalityLabel1_3Top, modalityLabel3width, modalityLabelHeight);

	metrics = QFontMetrics(m_modalityWeightFont);
	int modalityWeight3Width = metrics.width(m_modalityWeight3);
	m_modalityWeight1Pos = QPoint(modalityLabel1Left + modalityLabel1width + MODALITY_LABEL_MARGIN, modalityLabel1_3Bottom);
	m_modalityWeight2Pos = QPoint(modalityLabel2Left + modalityLabel2width + MODALITY_LABEL_MARGIN, modalityLabel2Bottom);
	m_modalityWeight3Pos = QPoint(modalityLabel3Left - MODALITY_LABEL_MARGIN - modalityWeight3Width, modalityLabel1_3Bottom);
	// }

	updateControlPointPosition();
	if (m_triangleRenderer) {
		m_triangleRenderer->setTriangle(m_triangle);
	}
}

void iABarycentricTriangleWidget::updateControlPointCoordinates(BCoord bCoord)
{
	updateControlPointPosition(m_triangle.getCartesianCoordinates(bCoord));
}

void iABarycentricTriangleWidget::updateControlPointPosition()
{
	updateControlPointPosition(m_triangle.getCartesianCoordinates(m_controlPointBCoord));
}

void iABarycentricTriangleWidget::updateControlPointPosition(QPoint newPos)
{
	BCoord bCoord = m_triangle.getBarycentricCoordinates(newPos.x(), newPos.y());

	if (bCoord.isInside())
	{
		m_controlPointBCoord = bCoord;
		moveControlPointTo(newPos);
		updateModalityWeightLabels(bCoord);
		update();
		emit weightChanged(bCoord);
	}
	else {
		// Do nothing for now
		// TODO: Set point to closest positiont inside the triangle
	}
}

void iABarycentricTriangleWidget::updateModalityWeightLabels(BCoord bCoord)
{
	// To format double values {
	//QString text; // Source: https://stackoverflow.com/questions/7234824/format-a-number-to-a-specific-qstring-format
	//QString modalityWeight3 = text.sprintf(WEIGHT_FORMAT, bCoord.getGamma() * 100);

	//QFontMetrics metrics = QFontMetrics(m_modalityWeightFont);
	//int modalityWeight3Width = metrics.width(modalityWeight3);
	//m_modalityWeight3Pos = QPoint(m_modalityLabel3Pos.x() - MODALITY_LABEL_MARGIN - modalityWeight3Width, m_modalityLabel3Pos.y());
	
	//m_modalityWeight1 = text.sprintf(WEIGHT_FORMAT, bCoord.getAlpha() * 100);
	//m_modalityWeight2 = text.sprintf(WEIGHT_FORMAT, bCoord.getBeta() * 100);
	//m_modalityWeight3 = modalityWeight3;
	// }

	// To format int values {
	int a = qRound(bCoord.getAlpha() * 100);
	int b = qRound(bCoord.getBeta() * 100);
	int c = 100 - a - b;

	QString modalityWeight3 = QString::number(c) + "%";
	QFontMetrics metrics = QFontMetrics(m_modalityWeightFont);
	int modalityWeight3Width = metrics.width(modalityWeight3);
	m_modalityWeight3Pos = QPoint(m_modalityLabel3Pos.x() - MODALITY_LABEL_MARGIN - modalityWeight3Width, m_modalityLabel3Pos.y());

	m_modalityWeight1 = QString::number(a) + "%";
	m_modalityWeight2 = QString::number(b) + "%";
	m_modalityWeight3 = modalityWeight3;
	// }
}

void iABarycentricTriangleWidget::moveControlPointTo(QPoint newPos)
{
	m_controlPointOld.setX(m_controlPoint.x());
	m_controlPointOld.setY(m_controlPoint.y());

	m_controlPoint.setX(newPos.x());
	m_controlPoint.setY(newPos.y());

	int movx = m_controlPoint.x() - m_controlPointOld.x();
	int movy = m_controlPoint.y() - m_controlPointOld.y();
	m_controlPointBorderPainterPath.translate(movx, movy);
	m_controlPointCrossPainterPath.translate(movx, movy);
}

bool iABarycentricTriangleWidget::interactWithModalityLabel(QPoint p, bool press)
{
	if (press) {
		if (m_modalityHighlightedIndex >= 0) {
			updateControlPointCoordinates(BCoord(m_modalityHighlightedIndex == 0 ? 1 : 0, m_modalityHighlightedIndex == 1 ? 1 : 0));
		}

	} else {
		for (int i = 0; i < 3; i++) {
			if (m_modalityLabelRect[i].contains(p)) {
				m_modalityHighlightedIndex = i;
				update();
				return true;
			}
		}
		if (m_modalityHighlightedIndex >= 0) {
			m_modalityHighlightedIndex = -1;
			update();
		}
	}
	return false;
}

bool iABarycentricTriangleWidget::isTooWide(int width, int height)
{
	// TODO (line above): casting width and height - is that really the best?
	return ((double)width / (double)height) < ONE_DIV_SIN60;
}

bool iABarycentricTriangleWidget::isTooTall(int width, int height)
{
	// TODO (line above): casting width and height - is that really the best?
	return ((double)width / (double)height) > ONE_DIV_SIN60;
}

bool iABarycentricTriangleWidget::hasWidthForHeight()
{
	return true;
}

int iABarycentricTriangleWidget::getWidthForHeight(int height)
{
	return (int)round(height * ONE_DIV_SIN60);
}

QWidget* iABarycentricTriangleWidget::widget()
{
	return this;
}

bool iABarycentricTriangleWidget::hasHeightForWidth()
{
	return true;
}

int iABarycentricTriangleWidget::getHeightForWidth(int width)
{
	return (int)round(width * SIN60);
}

int iABarycentricTriangleWidget::getWidthForCurrentHeight()
{
	return getWidthForHeight(height());
}

int iABarycentricTriangleWidget::getHeightForCurrentWidth()
{
	return getHeightForWidth(width());
}

void iABarycentricTriangleWidget::setFont(QFont font)
{
	m_modalityLabelFont = font;
}

void iABarycentricTriangleWidget::setModality1label(QString label)
{
	m_modalityLabel1 = label;
	recalculatePositions();
}

void iABarycentricTriangleWidget::setModality2label(QString label)
{
	m_modalityLabel2 = label;
	recalculatePositions();
}

void iABarycentricTriangleWidget::setModality3label(QString label)
{
	m_modalityLabel3 = label;
	recalculatePositions();
}

BCoord iABarycentricTriangleWidget::getWeight()
{
	return m_controlPointBCoord;
}

void iABarycentricTriangleWidget::setWeight(BCoord newWeight)
{
	updateControlPointCoordinates(newWeight);
}

void iABarycentricTriangleWidget::setTriangleRenderer(iATriangleRenderer *triangleRenderer)
{
	m_triangleRenderer = triangleRenderer;
}

void iABarycentricTriangleWidget::setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3)
{
	m_triangleRenderer->setModalities(d1, d2, d3, m_triangle);
}

// ----------------------------------------------------------------------------------------------
// PAINT METHODS
// ----------------------------------------------------------------------------------------------



void iABarycentricTriangleWidget::setBackgroundColor(QColor color)
{
	glClearColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0, color.alpha() / 255.0);
}

void iABarycentricTriangleWidget::initializeGL()
{
	setBackgroundColor(m_backgroundColor);
}

void iABarycentricTriangleWidget::resizeGL(int w, int h)
{
	recalculatePositions(w, h);
}

void iABarycentricTriangleWidget::paintGL()
{
	QPainter p(this);
	paintContext(p);
	paintControlPoint(p);
	paintModalityLabels(p);
}


void iABarycentricTriangleWidget::paintTriangleFill(QPainter &p)
{
	p.fillPath(m_trianglePainterPath, m_triangleFillBrush);
}

void iABarycentricTriangleWidget::paintTriangleBorder(QPainter &p)
{
	p.setPen(m_triangleBorderPen);
	p.drawPath(m_trianglePainterPath);
}

void iABarycentricTriangleWidget::paintControlPoint(QPainter &p)
{
	p.setPen(m_controlPointCrossPen);
	p.drawPath(m_controlPointCrossPainterPath);

	p.setPen(m_controlPointBorderPen);
	p.drawPath(m_controlPointBorderPainterPath);
}

void iABarycentricTriangleWidget::paintModalityLabels(QPainter &p)
{
	p.setFont(m_modalityLabelFont);
	p.drawText(m_modalityLabel1Pos, m_modalityLabel1);
	p.drawText(m_modalityLabel2Pos, m_modalityLabel2);
	p.drawText(m_modalityLabel3Pos, m_modalityLabel3);
	if (m_modalityHighlightedIndex >= 0) {
		p.setPen(m_modalityLabelHighlightPen);
		p.drawRect(m_modalityLabelRect[m_modalityHighlightedIndex]);
	}

	p.setFont(m_modalityWeightFont);
	p.drawText(m_modalityWeight1Pos, m_modalityWeight1);
	p.drawText(m_modalityWeight2Pos, m_modalityWeight2);
	p.drawText(m_modalityWeight3Pos, m_modalityWeight3);
}

void iABarycentricTriangleWidget::paintContext(QPainter &p) {
	if (m_triangleRenderer && m_triangleRenderer->canPaint()) {
		m_triangleRenderer->paintContext(p);
	}
	else {
		paintTriangleBorder(p);
		paintTriangleFill(p);
	}
}