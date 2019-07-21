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

#include "iABarycentricTriangleWidget.h"

#include "iABarycentricContextRenderer.h"

#include <vtkMath.h>

#include <QApplication>    // TODO: really necessary? (just to get the font()!)
#include <QDebug>    // Debug
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QString>
#include <QSpinBox>

// Constants (more in the header file!)
static const qreal RAD60 = vtkMath::Pi() / 3.0;
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

iABarycentricTriangleWidget::iABarycentricTriangleWidget(QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	m_controlPointBorderPen.setWidth(3);
	m_controlPointBorderPen.setColor(Qt::red);

	m_controlPointCrossPen.setWidth(2);
	m_controlPointCrossPen.setColor(Qt::red);

	//setMouseTracking(true); // to enable mouse move events without the mouse button needing to be pressed

	initializeControlPointPaths();

	QString mod[3] = { "A: ", "B: ", "C: " };
	for (int i = 0; i < 3; i++) {
		auto sb = m_spinBoxes[i] = new QSpinBox(this);
		sb->setRange(0, 100);
		sb->setSingleStep(1);
		sb->setSuffix("%");
		sb->setPrefix(mod[i]);
		sb->setStyleSheet("font-size: 10pt;");
	}

	connect(m_spinBoxes[0], SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged_1(int)));
	connect(m_spinBoxes[1], SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged_2(int)));
	connect(m_spinBoxes[2], SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged_3(int)));
}

void iABarycentricTriangleWidget::onSpinBoxValueChanged_1(int newValue) {
	int A = newValue;
	double a = A / 100.0;
	double rest = 1 - a;
	
	BCoord bc = getWeight();
	double b = bc[1];
	double c = bc[2];
	double sum = b + c;
	if (sum == 0) {
		b = rest / 2;
	} else {
		b = b / sum * rest;
	}

	int B = qRound(b * 100);
	int C = 100 - A - B;

	bc = BCoord(a, b);
	updateControlPointCoordinates(bc, A, B, C);
}

void iABarycentricTriangleWidget::onSpinBoxValueChanged_2(int newValue) {
	int B = newValue;
	double b = B / 100.0;
	double rest = 1 - b;

	BCoord bc = getWeight();
	double a = bc[0];
	double c = bc[2];
	double sum = a + c;
	if (sum == 0) {
		a = rest / 2;
	} else {
		a = a / sum * rest;
	}

	int A = qRound(a * 100);
	int C = 100 - A - B;

	bc = BCoord(a, b);
	updateControlPointCoordinates(bc, A, B, C);
}

void iABarycentricTriangleWidget::onSpinBoxValueChanged_3(int newValue) {
	int C = newValue;
	double c = C / 100.0;
	double rest = 1 - c;
	
	BCoord bc = getWeight();
	double a = bc[0];
	double b = bc[1];
	double sum = a + b;
	if (sum == 0) {
		a = rest / 2;
	} else {
		a = a / sum * rest;
	}
	b = 1 - a - c;

	int A = qRound(a * 100);
	int B = 100 - A - C;

	bc = BCoord(a, b);
	updateControlPointCoordinates(bc, A, B, C);
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

void iABarycentricTriangleWidget::mousePressEvent(QMouseEvent *event)
{
	updateControlPointPosition(event->pos());
	m_dragging = true;
}

void iABarycentricTriangleWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_dragging) {
		updateControlPointPosition(event->pos());
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
	int spinBoxHeight = m_spinBoxes[0]->sizeHint().height();
	
	int triangleSpacingLeft = MODALITY_LABEL_MARGIN; // LEFT margin of BOTTOM-LEFT modality
	int triangleSpacingTop = spinBoxHeight + MODALITY_LABEL_MARGIN_TIMES_TWO; // complete height of TOP modality
	int triangleSpacingRight = MODALITY_LABEL_MARGIN; // RIGHT margin of BOTTOM-RIGHT modality
	int triangleSpacingBottom = triangleSpacingTop; // complete height of BOTTOM modality

	int aw = width - triangleSpacingLeft - triangleSpacingRight; // available width (for the triangle)
	int ah = height - triangleSpacingTop - triangleSpacingBottom; // available height (for the triangle)
	int tw, th; // triangle's width and height
	if (isTooWide(aw, ah)) {
		tw = aw;
		th = getHeightForWidth(aw);
	} else {
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

	// QSpinnerBox
	{
		auto sb1 = m_spinBoxes[0];
		auto sb2 = m_spinBoxes[1];
		auto sb3 = m_spinBoxes[2];

		QSize size1 = sb1->sizeHint();
		QSize size2 = sb2->sizeHint();
		QSize size3 = sb3->sizeHint();

		QRect r1, r2, r3;
		if (!changeTriangle) {
			// TRIANGLE MODE
			r1 = QRect(m_triangle.getXa() - MODALITY_LABEL_MARGIN - size1.width(), m_triangle.getYa() - size1.height(), size1.width(), size1.height());
			r2 = QRect(m_triangle.getXb() + MODALITY_LABEL_MARGIN, m_triangle.getYb() - size2.height(), size2.width(), size2.height());
			r3 = QRect(m_triangle.getXc() - (size3.width() / 2), m_triangle.getYc() + MODALITY_LABEL_MARGIN, size3.width(), size3.height());

		} else {
			// STACK MODE
			r1 = QRect(left, bottom + MODALITY_LABEL_MARGIN, size1.width(), size1.height());
			r2 = QRect(centerX - (size2.width() / 2), top - MODALITY_LABEL_MARGIN - size2.height(), size2.width(), size2.height());
			r3 = QRect(right - size3.width(), bottom + MODALITY_LABEL_MARGIN, size3.width(), size3.height());
		}

		sb1->setGeometry(r1);
		sb2->setGeometry(r2);
		sb3->setGeometry(r3);
	}

	updateControlPointPosition();
	if (m_triangleRenderer) {
		m_triangleRenderer->setTriangle(m_triangle);
	}
}

void iABarycentricTriangleWidget::updateControlPoint(BCoord bCoord, QPoint newPos, int a, int b, int c)
{
	if (!bCoord.isInside()) {
		// Snap to edge

		double a = bCoord.getAlpha();
		double b = bCoord.getBeta();
		double c = bCoord.getGamma();

		a = a < 0 ? 0 : a;
		b = b < 0 ? 0 : b;
		c = c < 0 ? 0 : c;
		double sum = a + b + c;
		if (sum == 0) {
			// No idea if this can possibly happen
			// Probably good to be safe though
			return;
		}

		a /= sum;
		b /= sum;
		//c /= sum;

		bCoord = BCoord(a, b);
		updateControlPointCoordinates(bCoord);
		return;
	}

	int abc[3] = { a, b, c };
	for (int i = 0; i < 3; i++) {
		auto sb = m_spinBoxes[i];
		QSignalBlocker blocker(sb);
		sb->setValue(abc[i]);
	}

	m_controlPointBCoord = bCoord;
	moveControlPointTo(newPos);
	update();
	emit weightsChanged(bCoord);
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

int iABarycentricTriangleWidget::getWidthForHeight(int height)
{
	return (int)round(height * ONE_DIV_SIN60);
}

int iABarycentricTriangleWidget::getHeightForWidth(int width)
{
	return (int)round(width * SIN60);
}

BCoord iABarycentricTriangleWidget::getWeight()
{
	return m_controlPointBCoord;
}

void iABarycentricTriangleWidget::setWeight(BCoord newWeight)
{
	updateControlPointCoordinates(newWeight);
}

void iABarycentricTriangleWidget::setTriangleRenderer(iABarycentricContextRenderer *triangleRenderer)
{
	if (m_triangleRenderer) {
		disconnect(m_triangleRenderer, SIGNAL(heatmapReady()), this, SLOT(onHeatmapReady()));
	}
	m_triangleRenderer = triangleRenderer;
	if (m_triangleRenderer) {
		connect(m_triangleRenderer, SIGNAL(heatmapReady()), this, SLOT(onHeatmapReady()));
	}
}

void iABarycentricTriangleWidget::setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3, QString const name[3])
{
	m_triangleRenderer->setModalities(d1, d2, d3, m_triangle);
	for (int i = 0; i < 3; ++i)
		m_spinBoxes[i]->setPrefix(QString("%1: ").arg(name[i]));
}

void iABarycentricTriangleWidget::resizeEvent(QResizeEvent* event)
{
	recalculatePositions(event->size().width(), event->size().height());
}

void iABarycentricTriangleWidget::onHeatmapReady() {
	update();
}

// ----------------------------------------------------------------------------------------------
// PAINT METHODS
// ----------------------------------------------------------------------------------------------

void iABarycentricTriangleWidget::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	paintContext(p);
	paintTriangleBorder(p);
	paintControlPoint(p);
	//paintModalityLabels(p);
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

void iABarycentricTriangleWidget::paintContext(QPainter &p) {
	QImage *img = m_triangleRenderer->getImage();
	QSize size = img->size();

	if (img->isNull() || size.width() == 0 || size.height() == 0) {
		paintTriangleBorder(p);
		paintTriangleFill(p);
	} else {
		QRect rect = m_triangleRenderer->getImageRect();

		if (size != rect.size()) {
			p.drawImage(rect, img->scaled(rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		} else {
			p.drawImage(rect, *img, img->rect());;
		}
	}
}