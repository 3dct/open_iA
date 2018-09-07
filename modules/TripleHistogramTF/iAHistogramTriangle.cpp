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
* program.  If not, see http://aw.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iAHistogramTriangle.h"
#include "charts/iADiagramFctWidget.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QtMath>

// Debug
#include <QDebug>
#include "iASlicerData.h"
#include "iASlicerWidget.h"

const static qreal RAD60 = qDegreesToRadians(60.0);
const static qreal SIN60 = qSin(RAD60);
const static qreal COS60 = qCos(RAD60);
const static int HISTOGRAM_HEIGHT = 100;
const static int TRIANGLE_LEFT = qRound(SIN60 * HISTOGRAM_HEIGHT);
const static int TRIANGLE_TOP = qRound(COS60 * HISTOGRAM_HEIGHT);
const static int TRIANGLE_RIGHT = TRIANGLE_LEFT;
const static int TRIANGLE_BOTTOM = HISTOGRAM_HEIGHT;
const static double TRIANGLE_HEIGHT_RATIO = SIN60;
const static double TRIANGLE_WIDTH_RATIO = 1.0 / TRIANGLE_HEIGHT_RATIO;

iAHistogramTriangle::iAHistogramTriangle(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f)
	: iATripleModalityWidget(parent, mdiChild, f)
{
}

void iAHistogramTriangle::initialize()
{
	m_clipPathPen.setWidth(1);
	m_clipPathPen.setColor(Qt::black);

	m_slicerBackgroundBrush.setColor(Qt::black);

	setMouseTracking(true);

	QWidget *widget = new QWidget(this);
	//temp->setVisible(false);
	QLayout *layout = new QHBoxLayout(widget);
	layout->addWidget(m_slicerWidgets[0]);
	layout->addWidget(m_slicerWidgets[1]);
	layout->addWidget(m_slicerWidgets[2]);
	//layout->setGeometry(QRect(0, 0, 300, 300));
	//layout->setMargin(300);

	//QLayout *thisLayout = new QHBoxLayout(this);
	//thisLayout->addWidget(temp);

	calculatePositions();
}

void iAHistogramTriangle::setModalityLabel(QString label, int index)
{
	//if (isReady()) {
	//	m_modalityLabels[index]->setText(label);
		iATripleModalityWidget::setModalityLabel(label, index);
	//}
}

void iAHistogramTriangle::resizeEvent(QResizeEvent* event)
{
	calculatePositions(event->size().width(), event->size().height());
}

// EVENTS ----------------------------------------------------------------------------------------------------------

void iAHistogramTriangle::forwardMouseEvent(QMouseEvent *event)
{
	QWidget *slicerTarget;
	QPoint transformed;

	slicerTarget = onSlicer(event->pos(), transformed);
	if (slicerTarget != nullptr && dragging) {
		int deltaX = event->pos().x() - lastMousePos.x();
		int deltaY = event->pos().y() - lastMousePos.y();
		if (deltaX != 0 || deltaY != 0) {
			int index = -1;
			if (m_slicerWidgets[0] == slicerTarget) {
				index = 0;
			} else if (m_slicerWidgets[1] == slicerTarget) {
				index = 1;
			} else if (m_slicerWidgets[2] == slicerTarget) {
				index = 2;
			}
			if (index > -1) {
				m_transformSlicers[index].translate(deltaX, deltaY);
			}
		}
		update();
		lastTarget = slicerTarget;
		return;
	}

	if (onTriangle(event->pos())) {
		QApplication::sendEvent(m_triangleWidget, event);
		update();
		lastTarget = m_triangleWidget;
		return;
	}

	QWidget *target;

	if ((target = onHistogram(event->pos(), transformed)) != nullptr) {
		event->setLocalPos(transformed);
		QApplication::sendEvent(target, event);
		update();
		lastTarget = target;
		return;
	}

	// Forwarding won't work because we're drawing the framebuffer of the slicer... TODO after fixing that, uncomment this
	/*if ((target = onSlicer(event->pos(), transformed)) != nullptr) {
		event->setLocalPos(transformed);
		QApplication::sendEvent(target, event);
		update();
		lastTarget = target;
		return;
	}*/
	// ...instead, tranlate the slicer's transform
	if (event->button() == Qt::MiddleButton && slicerTarget != nullptr) {
		dragging = true;
		lastTarget = slicerTarget;
	}
}

void iAHistogramTriangle::forwardWheelEvent(QWheelEvent *e)
{
	QPoint transformed;
	iASimpleSlicerWidget *target;
	if ((target = onSlicer(e->pos(), transformed)) != nullptr) {
		QWheelEvent *newE = new QWheelEvent(e->posF(), e->globalPosF(), e->pixelDelta(), e->angleDelta(),
			e->delta(), e->orientation(), e->buttons(),
			e->modifiers(), e->phase(), e->source(), e->inverted());
		QApplication::sendEvent(target, newE);
		update();
		return;
	}
}

iADiagramFctWidget* iAHistogramTriangle::onHistogram(QPoint p, QPoint &transformed)
{
	for (int i = 0; i < 3; i++) {
		transformed = m_transformHistograms[i].inverted().map(p);
		if (m_histogramsRect.contains(transformed)) {
			return m_histograms[i];
		}
	}
	return nullptr;
}

bool iAHistogramTriangle::onTriangle(QPoint p)
{
	return m_triangleWidget->getTriangle().contains(p.x(), p.y());
}

iASimpleSlicerWidget* iAHistogramTriangle::onSlicer(QPoint p, QPoint &transformed)
{
	for (int i = 0; i < 3; i++) {
		if (m_slicerTriangles[i].contains(p.x(), p.y())) {
			transformed = m_transformSlicers[i].inverted().map(p);
			return m_slicerWidgets[i];
		}
	}
	return nullptr;
}

// POSITION AND PAINT ----------------------------------------------------------------------------------------------

void iAHistogramTriangle::calculatePositions(int totalWidth, int totalHeight)
{
	int boxLeft, boxTop, boxRight, boxBottom;
	int left, top, right, bottom, centerX, width, height; // Big triangle's positions
	int w, h, l, t, r; // Small triangle's positions

	{ // Triangle positions
		int aw  = totalWidth - TRIANGLE_LEFT - TRIANGLE_RIGHT; // Available width for the triangle
		int ah = totalHeight - TRIANGLE_TOP - TRIANGLE_BOTTOM;

		if ((double)aw / (double)ah < TRIANGLE_WIDTH_RATIO) {
			width = aw;
			height = qRound(aw * TRIANGLE_HEIGHT_RATIO);
		} else {
			width = qRound(ah * TRIANGLE_WIDTH_RATIO);
			height = ah;
		}

		// The box will bound all the elements (triangle, histograms, slices)
		boxLeft = (aw - width) >> 1;
		boxTop = (ah - height) >> 1;
		boxRight = boxLeft + width + TRIANGLE_LEFT + TRIANGLE_RIGHT;
		boxBottom = boxTop + height + TRIANGLE_TOP + TRIANGLE_BOTTOM;

		//        /\ 
		//       /  \    BIG TRIANGLE
		//      /    \   bounding box: left, top, right, bottom (, centerX)
		//     /______\ 
		left = boxLeft + TRIANGLE_LEFT; // division by 2 = right shift by 1
		top = boxTop + TRIANGLE_TOP;
		right = left + width;
		bottom = top + height;
		centerX = left + (width >> 1);

		//        /\           /\
		//       /  \         /__\       ____   SMALL TRIANGLE
		//      /    \  -->  /\  /\  --> \  /   bounding box: l, t, r, b (, cX)
		//     /______\     /__\/__\      \/
		w = width >> 1;
		h = height >> 1;
		l = (left + centerX) >> 1;
		t = top + h;
		r = l + w;
		//int b = bottom;
		//int cx = centerX;

		m_triangleWidget->recalculatePositions(w, h, BarycentricTriangle(l, t, r, t, centerX, bottom));
		m_triangleBigWidth = width;
	}

	{ // Set up the histograms' transforms and resize them
		m_transformHistograms[0].reset(); // left
		m_transformHistograms[0].translate(left - TRIANGLE_LEFT, bottom - TRIANGLE_TOP);
		m_transformHistograms[0].rotate(-60.0);

		m_transformHistograms[1].reset(); // right
		m_transformHistograms[1].translate(centerX + TRIANGLE_LEFT, top - TRIANGLE_TOP);
		m_transformHistograms[1].rotate(60.0);

		m_transformHistograms[2].reset(); // bottom
		m_transformHistograms[2].translate(left, bottom);

		QSize size = QSize(width, HISTOGRAM_HEIGHT);
		m_histogramsRect = QRect(0, 0, width, HISTOGRAM_HEIGHT);
		m_histograms[0]->resize(size);
		m_histograms[1]->resize(size);
		m_histograms[2]->resize(size);
	}

	{ // Set up the slicers's transforms and resize them
		m_transformSlicers[0].reset(); // left
		m_transformSlicers[0].translate(left, t);
		m_slicerTriangles[0].set(left, bottom, l, t, centerX, bottom);

		m_transformSlicers[1].reset(); // top
		m_transformSlicers[1].translate(l, top);
		m_slicerTriangles[1].set(l, t, centerX, top, r, t);

		m_transformSlicers[2].reset(); // right
		m_transformSlicers[2].translate(centerX, t);
		m_slicerTriangles[2].set(centerX, bottom, r, t, right, bottom);

		// Path encloses the left small triangle. When drawing, the path can be translated
		// to the other triangle's positions, then translated back to the initial position
		m_slicerClipPaths[0] = QPainterPath();
		m_slicerClipPaths[0].moveTo(QPointF(l, t));
		m_slicerClipPaths[0].lineTo(QPointF(centerX, bottom));
		m_slicerClipPaths[0].lineTo(QPointF(left, bottom));
		m_slicerClipPaths[0].lineTo(QPointF(l, t));

		m_slicerClipPaths[1] = m_slicerClipPaths[0];
		m_slicerClipPaths[1].translate(w >> 1, -h);

		m_slicerClipPaths[2] = m_slicerClipPaths[0];
		m_slicerClipPaths[2].translate(w, 0);

		QRect rect = QRect(0, 0, w, h);
		m_slicerWidgets[0]->setGeometry(rect);
		m_slicerWidgets[1]->setGeometry(rect);
		m_slicerWidgets[2]->setGeometry(rect);
		QSize size = QSize(w, h);
		m_slicerWidgets[0]->getSlicer()->widget()->resize(size);
		m_slicerWidgets[1]->getSlicer()->widget()->resize(size);
		m_slicerWidgets[2]->getSlicer()->widget()->resize(size);
	}

	{ // Set up the clipping path
		int histoLateralY = bottom - TRIANGLE_TOP;
		m_clipPath = QPainterPath();
		m_clipPath.moveTo(left, bottom);
		m_clipPath.lineTo(boxLeft, histoLateralY);
		m_clipPath.lineTo(centerX - TRIANGLE_LEFT, boxTop);
		m_clipPath.lineTo(centerX, top);
		m_clipPath.lineTo(centerX + TRIANGLE_LEFT, boxTop);
		m_clipPath.lineTo(boxRight, histoLateralY);
		m_clipPath.lineTo(right, bottom);
		m_clipPath.lineTo(right, boxBottom);
		m_clipPath.lineTo(left, boxBottom);
		m_clipPath.lineTo(left, bottom);
	}
}

void iAHistogramTriangle::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	p.setRenderHint(QPainter::RenderHint::Antialiasing);
	//p.setClipPath(m_clipPath);

	paintSlicers(p);
	m_triangleWidget->paintContext(p);
	paintHistograms(p);
	//m_triangleWidget->paintTriangleBorder(p);
	m_triangleWidget->paintControlPoint(p);

	//p.setClipping(false);
	p.setPen(m_clipPathPen);
	p.drawPath(m_clipPath);
}

void iAHistogramTriangle::paintSlicers(QPainter &p)
{
	bool hasClipping = p.hasClipping();
	QPainterPath oldClipPath = p.clipPath();

	p.setClipping(false);
	p.fillPath(m_slicerClipPaths[0], QBrush(Qt::black)); // TODO use m_slicerBackgroundBrush
	p.fillPath(m_slicerClipPaths[1], QBrush(Qt::black));
	p.fillPath(m_slicerClipPaths[2], QBrush(Qt::black));

	QImage img;
	for (int i = 0; i < 3; i++) {
		p.setClipPath(m_slicerClipPaths[i]);
		p.setTransform(m_transformSlicers[i]);
		img = m_slicerWidgets[i]->getSlicer()->widget()->grabFrameBuffer();
		p.drawImage(0, 0, img);
		p.resetTransform(); // otherwise, setClipPath will transform as well
	}

	if (hasClipping) {
		p.setClipPath(oldClipPath);
	} else {
		p.setClipping(false);
	}
}

void iAHistogramTriangle::paintHistograms(QPainter &p)
{
	//QPen oldPen = p.pen();
	//p.setPen(m_histogramsBorderPen);

	// LEFT
	p.setTransform(m_transformHistograms[0]);
	m_histograms[0]->render(&p);
	//p.drawRect(m_histogramsRect);

	// RIGHT
	p.setTransform(m_transformHistograms[1]);
	m_histograms[1]->render(&p);
	//p.drawRect(m_histogramsRect);

	// BOTTOM
	p.setTransform(m_transformHistograms[2]);
	m_histograms[2]->render(&p);
	//p.drawRect(m_histogramsRect);

	p.resetTransform();
	//p.setPen(oldPen);
}