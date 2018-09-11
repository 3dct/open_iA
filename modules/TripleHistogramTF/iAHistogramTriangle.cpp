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

	setAttribute(Qt::WA_OpaquePaintEvent, true);

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

	m_sliceSlider->setOrientation(Qt::Vertical);
	m_sliceSlider->setInvertedAppearance(true); // top to bottom, consistent with the main sliders's scroll bars
	m_sliceSlider->setParent(this);

	m_slicerModeComboBox->setParent(this);

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
	if (dragging /*&& event->button() == Qt::MidButton*/) {
		int deltaX = event->pos().x() - lastMousePos.x();
		int deltaY = event->pos().y() - lastMousePos.y();
		if (deltaX != 0 || deltaY != 0) {
			int index = -1;
			if (m_slicerWidgets[0] == lastTarget) {
				index = 0;
			} else if (m_slicerWidgets[1] == lastTarget) {
				index = 1;
			} else if (m_slicerWidgets[2] == lastTarget) {
				index = 2;
			}
			if (index > -1) {
				m_transformSlicers[index].translate(deltaX, deltaY);
				m_fRenderSlicer[index] = true;
				update();
			}
		}
		return;
	}

	if (onTriangle(event->pos())) {
		QApplication::sendEvent(m_triangleWidget, event);
		lastTarget = m_triangleWidget;
		m_fRenderTriangle = true;
		update();
		return;
	}

	QPoint transformed;
	int index;
	QWidget *target;

	if ((target = onHistogram(event->pos(), transformed, index))) {
		event->setLocalPos(transformed);
		QApplication::sendEvent(target, event);
		lastTarget = target;
		m_fRenderHistogram[index] = true;
		m_fRenderSlicer[index] = true;
		update();
		return;
	}

	// Forwarding won't work because we're drawing the framebuffer of the slicer... TODO after fixing that, uncomment this
	/*if ((target = onSlicer(event->pos(), transformed))) {
		event->setLocalPos(transformed);
		QApplication::sendEvent(target, event);
		update();
		lastTarget = target;
		return;
	}*/
	// ...instead, tranlate the slicer's transform
	target = onSlicer(event->pos(), transformed, index);
	if (target && event->button() == Qt::MidButton) {
		lastTarget = target;
		dragging = true;
		return;
	}
}

void iAHistogramTriangle::forwardWheelEvent(QWheelEvent *e)
{
	QPoint transformed;
	int index;
	iASimpleSlicerWidget *target;
	if ((target = onSlicer(e->pos(), transformed, index))) {
		QWheelEvent *newE = new QWheelEvent(e->posF(), e->globalPosF(), e->pixelDelta(), e->angleDelta(),
			e->delta(), e->orientation(), e->buttons(),
			e->modifiers(), e->phase(), e->source(), e->inverted());
		QApplication::sendEvent(target->getSlicer()->widget(), newE);
		m_fRenderSlicer[index] = true;
		update();
		return;
	}
}

void iAHistogramTriangle::forwardContextMenuEvent(QContextMenuEvent *e)
{
	QPoint transformed;
	int index;
	iADiagramFctWidget* target = onHistogram(e->pos(), transformed, index);
	if (target) {
		QContextMenuEvent *newE = new QContextMenuEvent(e->reason(), transformed, e->globalPos());
		QApplication::sendEvent(target, newE);
		//m_fRenderHistogram[index] = true;
		//update();
		return;
	}
}

iADiagramFctWidget* iAHistogramTriangle::onHistogram(QPoint p, QPoint &transformed, int &index)
{
	for (int i = 0; i < 3; i++) {
		transformed = m_transformHistograms[i].inverted().map(p);
		if (m_histogramsRect.contains(transformed)) {
			index = i;
			return m_histograms[i];
		}
	}
	index = -1;
	return nullptr;
}

bool iAHistogramTriangle::onTriangle(QPoint p)
{
	return m_triangleWidget->getTriangle().contains(p.x(), p.y());
}

iASimpleSlicerWidget* iAHistogramTriangle::onSlicer(QPoint p, QPoint &transformed, int &index)
{
	for (int i = 0; i < 3; i++) {
		if (m_slicerTriangles[i].contains(p.x(), p.y())) {
			transformed = m_transformSlicers[i].inverted().map(p);
			index = i;
			return m_slicerWidgets[i];
		}
	}
	index = -1;
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
		m_transformSlicers[0].reset(); // right
		m_transformSlicers[0].translate(centerX, t);
		m_slicerTriangles[0].set(centerX, bottom, r, t, right, bottom);

		m_transformSlicers[1].reset(); // left
		m_transformSlicers[1].translate(left, t);
		m_slicerTriangles[1].set(left, bottom, l, t, centerX, bottom);

		m_transformSlicers[2].reset(); // top
		m_transformSlicers[2].translate(l, top);
		m_slicerTriangles[2].set(l, t, centerX, top, r, t);


		// Path encloses the left small triangle. When drawing, the path can be translated
		// to the other triangle's positions, then translated back to the initial position
		m_slicerClipPaths[1] = QPainterPath(); // left
		m_slicerClipPaths[1].moveTo(QPointF(l, t));
		m_slicerClipPaths[1].lineTo(QPointF(centerX, bottom));
		m_slicerClipPaths[1].lineTo(QPointF(left, bottom));
		m_slicerClipPaths[1].lineTo(QPointF(l, t));

		m_slicerClipPaths[2] = m_slicerClipPaths[1]; // top
		m_slicerClipPaths[2].translate(w >> 1, -h);

		m_slicerClipPaths[0] = m_slicerClipPaths[1]; // right
		m_slicerClipPaths[0].translate(w, 0);

		QRect rect = QRect(0, 0, w, h);
		m_slicerWidgets[0]->setGeometry(rect);
		m_slicerWidgets[1]->setGeometry(rect);
		m_slicerWidgets[2]->setGeometry(rect);
		QSize size = QSize(w, h);
		m_slicerWidgets[0]->getSlicer()->widget()->resize(size);
		m_slicerWidgets[1]->getSlicer()->widget()->resize(size);
		m_slicerWidgets[2]->getSlicer()->widget()->resize(size);
	}

	int histoLateralY = bottom - TRIANGLE_TOP;
	int histoTop1X = centerX - TRIANGLE_LEFT;

	{ // Set up the clip path convering all widgets
		m_clipPath = QPainterPath();
		m_clipPath.moveTo(left, bottom);
		m_clipPath.lineTo(boxLeft, histoLateralY);
		m_clipPath.lineTo(histoTop1X, boxTop);
		m_clipPath.lineTo(centerX, top);
		m_clipPath.lineTo(centerX + TRIANGLE_LEFT, boxTop);
		m_clipPath.lineTo(boxRight, histoLateralY);
		m_clipPath.lineTo(right, bottom);
		m_clipPath.lineTo(right, boxBottom);
		m_clipPath.lineTo(left, boxBottom);
		m_clipPath.lineTo(left, bottom);
	}

	{ // Combo box and slider
		int controlsWidth = qMax(m_slicerModeComboBox->sizeHint().width(), m_sliceSlider->sizeHint().width());
		int controlsBottom = controlsWidth * (-histoLateralY / histoTop1X) + histoLateralY;
		int comboBoxHeight = m_slicerModeComboBox->sizeHint().height();
		int sliderHeight = controlsBottom - comboBoxHeight;

		m_slicerModeComboBox->setGeometry(QRect(0, 0, controlsWidth, comboBoxHeight));
		m_sliceSlider->setGeometry(QRect(0, m_slicerModeComboBox->sizeHint().height(), controlsWidth, sliderHeight));

		m_rControls = QRect(0, 0, controlsWidth, controlsBottom);
	}

	m_fClear = true;
}

void iAHistogramTriangle::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
	//p.setClipPath(m_clipPath);

	if (m_fClear) {
		p.eraseRect(0, 0, size().width(), size().height());
		m_fRenderHistogram[0] = true;
		m_fRenderHistogram[1] = true;
		m_fRenderHistogram[2] = true;
		m_fRenderSlicer[0] = true;
		m_fRenderSlicer[1] = true;
		m_fRenderSlicer[2] = true;
		m_fRenderTriangle = true;
	} else {
		p.eraseRect(m_rControls);
	}

	paintSlicers(p);

	if (m_fRenderTriangle) {
		m_triangleWidget->paintContext(p);
	}

	paintHistograms(p);

	// Always draw this, independent of any flags!
	//m_triangleWidget->paintTriangleBorder(p);
	m_triangleWidget->paintControlPoint(p);

	if (m_fRenderHistogram[0] || m_fRenderHistogram[1] || m_fRenderHistogram[2]) {
		//p.setClipping(false);
		p.setPen(m_clipPathPen);
		p.drawPath(m_clipPath);
	}

	m_fClear = false;
	m_fRenderHistogram[0] = false;
	m_fRenderHistogram[1] = false;
	m_fRenderHistogram[2] = false;
	m_fRenderSlicer[0] = false;
	m_fRenderSlicer[1] = false;
	m_fRenderSlicer[2] = false;
	m_fRenderTriangle = false;
}

void iAHistogramTriangle::paintSlicers(QPainter &p)
{
	bool hasClipping = p.hasClipping();
	QPainterPath oldClipPath = p.clipPath();

	QImage img;
	for (int i = 0; i < 3; i++) {
		if (m_fRenderSlicer[i]) {
			p.setClipping(false);
			p.fillPath(m_slicerClipPaths[i], QBrush(Qt::black)); // TODO use m_slicerBackgroundBrush

			p.setClipPath(m_slicerClipPaths[i]);
			p.setTransform(m_transformSlicers[i]);
			img = m_slicerWidgets[i]->getSlicer()->widget()->grabFrameBuffer();
			p.drawImage(0, 0, img);

			p.resetTransform(); // otherwise, clip path in setClipPath will be transformed as well
		}
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
	if (m_fRenderHistogram[0]) {
		p.setTransform(m_transformHistograms[0]);
		m_histograms[0]->render(&p);
		//p.drawRect(m_histogramsRect);
	}

	// RIGHT
	if (m_fRenderHistogram[1]) {
		p.setTransform(m_transformHistograms[1]);
		m_histograms[1]->render(&p);
		//p.drawRect(m_histogramsRect);
	}

	// BOTTOM
	if (m_fRenderHistogram[2]) {
		p.setTransform(m_transformHistograms[2]);
		m_histograms[2]->render(&p);
		//p.drawRect(m_histogramsRect);
	}

	p.resetTransform();
	//p.setPen(oldPen);
}