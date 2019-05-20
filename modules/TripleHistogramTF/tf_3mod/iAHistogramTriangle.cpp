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
* program.  If not, see http://aw.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iAHistogramTriangle.h"

#include "iABarycentricTriangleWidget.h"
#include "iATripleModalityWidget.h"
#include "iASimpleSlicerWidget.h"

#include "iASlicerData.h"
#include "iASlicerWidget.h"

#include <charts/iADiagramFctWidget.h>
#include <iASlicerData.h>
#include <iASlicerWidget.h>

#include <QPoint>
#include <QApplication>
#include <QComboBox>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QSlider>
#include <QCheckBox>
#include <QtMath>
#include <QLabel>

// Debug
#include <QDebug>

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
const static int MODALITY_LABEL_MARGIN = 10;

iAHistogramTriangle::iAHistogramTriangle(QWidget* parent, iATripleModalityWidget* tripleModalityWidget, MdiChild *mdiChild, Qt::WindowFlags f)
	: m_tmw(tripleModalityWidget)
{
}

void iAHistogramTriangle::glresized() {
	qDebug() << "GL RESIZED!";
}

void iAHistogramTriangle::initialize()
{
	m_clipPathPen.setWidth(1);
	m_clipPathPen.setColor(Qt::black);

	m_slicerBackgroundBrush.setColor(Qt::black);

	setMouseTracking(true);

	QWidget *widget = new QWidget(this);
	QLayout *layout = new QHBoxLayout(widget);
	for (int i = 0; i < 3; i++) {
		layout->addWidget(m_tmw->w_slicer(i)->getSlicer()->widget());

		// The following call will make sure that
		// m_tmw->m_slicerWidgets[i]->getSlicer()->widget()->isValid()
		// returns true (see isValid() definition 19/04/2019)
		// That prevents a "Framebuffer incomplete attachment" warning
		m_tmw->w_slicer(i)->getSlicer()->widget()->grabFramebuffer();
	}

	//QLayout *thisLayout = new QHBoxLayout(this);
	//thisLayout->addWidget(temp);

	m_tmw->w_layoutComboBox()->setParent(this);
	m_tmw->w_sliceNumberLabel()->setParent(this);
	m_tmw->w_slicerModeLabel()->setParent(this);
	m_tmw->w_checkBox_weightByOpacity()->setParent(this);

	calculatePositions();
	m_fClear = true;
	update();

	// Debug
	for (int i = 0; i < 3; i++) {
		connect(m_tmw->w_slicer(i)->getSlicer()->widget(), SIGNAL(resized()), this, SLOT(glresized()));
	}

	// CONNECTIONS
	connect(m_tmw, SIGNAL(transferFunctionChanged()), this, SLOT(updateSlicers()));
	connect(m_tmw, SIGNAL(slicerModeChanged(iASlicerMode)), this, SLOT(updateSlicers()));
	connect(m_tmw, SIGNAL(sliceNumberChanged(int)), this, SLOT(updateSlicers()));
	connect(m_tmw, SIGNAL(slicerModeChangedExternally(iASlicerMode)), this, SLOT(updateSlicers()));
	connect(m_tmw, SIGNAL(sliceNumberChangedExternally(int)), this, SLOT(updateSlicers()));

	connect(m_tmw, SIGNAL(transferFunctionChanged()), this, SLOT(updateHistograms()));
}

void iAHistogramTriangle::resizeEvent(QResizeEvent* event)
{
	calculatePositions(event->size().width(), event->size().height());
}

void iAHistogramTriangle::updateSlicers()
{
	for (int i = 0; i < 3; i++) {
		m_fRenderSlicer[i] = true;
	}
	update();
}

void iAHistogramTriangle::updateHistograms()
{
	for (int i = 0; i < 3; i++) {
		m_fRenderHistogram[i] = true;
	}
	update();
}

// EVENTS ----------------------------------------------------------------------------------------------------------

void iAHistogramTriangle::forwardMouseEvent(QMouseEvent *event, MouseEventType eventType)
{
	WidgetType widgetType = NONE;
	QPoint transformed = QPoint();
	QWidget *target = nullptr;
	if (m_draggedType == HISTOGRAM) {
		transformed = m_transformHistograms[m_lastIndex].inverted().map(event->pos());
	} else if (m_draggedType == SLICER) {
		transformed = m_transformSlicers[m_lastIndex].inverted().map(event->pos());
	}

	if (m_draggedType == TRIANGLE || (m_draggedType == NONE && onTriangle(event->pos()))) {
		m_fRenderTriangle = true;

		widgetType = TRIANGLE;
		target = m_tmw->w_triangle();
	}

	else if (m_draggedType == HISTOGRAM || (m_draggedType == NONE && (target = onHistogram(event->pos(), transformed).data()))) {
		event->setLocalPos(transformed);
		m_fRenderHistogram[m_lastIndex] = true; // m_lastIndex is affected by onHistogram function
		m_fRenderSlicer[m_lastIndex] = true; // m_lastIndex is affected by onHistogram function

		widgetType = HISTOGRAM;
		target = target ? target : m_draggedWidget;
	}

	else if (m_draggedType == SLICER || (m_draggedType == NONE && (target = onSlicer(event->pos(), transformed)))) {
		event->setLocalPos(transformed);
		m_fRenderSlicer[m_lastIndex] = true; // m_lastIndex is affected by onHistogram function

		widgetType = SLICER;
		target = target ? target : m_draggedWidget;
	}

	else {
		// the triangle widget actually occupies the whole screen, despite the triangle
		//     itself being smaller
		// the modality labels, for example, are part of the triangle widget
		// to keep the interaction, forward the event to the triangle widget here
		target = m_tmw->w_triangle();

		// BUT DON'T RENDER THE TRIANGLE!
		// The modality labels are rendered anyway, there's no need for this flag to be set
		//m_fRenderTriangle = true;
	}

	if (eventType == PRESS) {
		m_draggedType = widgetType;
		m_draggedWidget = target;
	} else if (eventType == RELEASE) {
		m_draggedType = NONE;
		m_draggedWidget = nullptr;
	}

	QApplication::sendEvent(target, event);
	update();
}

void iAHistogramTriangle::forwardWheelEvent(QWheelEvent *e)
{
	QPoint transformed;
	iASlicerWidget *target;
	if (target = onSlicer(e->pos(), transformed)) {
		QWheelEvent *newE = new QWheelEvent(transformed, e->globalPosF(), e->pixelDelta(), e->angleDelta(),
			e->delta(), e->orientation(), e->buttons(),
			e->modifiers(), e->phase(), e->source(), e->inverted());
		QApplication::sendEvent(target, newE);
		m_fRenderSlicer[m_lastIndex] = true;
		update();
		return;
	}
}

void iAHistogramTriangle::forwardContextMenuEvent(QContextMenuEvent *e)
{
	QPoint transformed;
	iADiagramFctWidget *target = onHistogram(e->pos(), transformed).data();
	if (target) {
		QContextMenuEvent *newE = new QContextMenuEvent(e->reason(), transformed, e->globalPos());
		QApplication::sendEvent(target, newE);
		//m_fRenderHistogram[index] = true;
		//update();
		return;
	}
}

QSharedPointer<iADiagramFctWidget> iAHistogramTriangle::onHistogram(QPoint p, QPoint &transformed)
{
	for (int i = 0; i < 3; i++) {
		transformed = m_transformHistograms[i].inverted().map(p);
		if (m_histogramsRect.contains(transformed)) {
			m_lastIndex = i;
			return m_tmw->w_histogram(i);
		}
	}
	m_lastIndex = -1;
	return nullptr;
}

bool iAHistogramTriangle::onTriangle(QPoint p)
{
	return m_tmw->w_triangle()->getTriangle().contains(p.x(), p.y());
}

iASlicerWidget* iAHistogramTriangle::onSlicer(QPoint p, QPoint &transformed)
{
	for (int i = 0; i < 3; i++) {
		if (m_slicerTriangles[i].contains(p.x(), p.y())) {
			transformed = m_transformSlicers[i].inverted().map(p);
			m_lastIndex = i;
			return m_tmw->w_slicer(i)->getSlicer()->widget();
		}
	}
	m_lastIndex = -1;
	return nullptr;
}

// POSITION AND PAINT ----------------------------------------------------------------------------------------------

void iAHistogramTriangle::calculatePositions(int totalWidth, int totalHeight)
{
	int boxLeft, boxTop, boxRight, boxBottom;
	int left, top, right, bottom, centerX, width, height; // Big triangle's positions
	int w, h, l, t, r; // Small triangle's positions

	// Triangle positions
	{
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
		boxLeft = (aw - width) / 2;
		boxTop = (ah - height) / 2;
		boxRight = boxLeft + width + TRIANGLE_LEFT + TRIANGLE_RIGHT;
		boxBottom = boxTop + height + TRIANGLE_TOP + TRIANGLE_BOTTOM;

		//        /\ 
		//       /  \    BIG TRIANGLE
		//      /    \   bounding box: left, top, right, bottom (, centerX)
		//     /______\ 
		left = boxLeft + TRIANGLE_LEFT;
		top = boxTop + TRIANGLE_TOP;
		right = left + width;
		bottom = top + height;
		centerX = left + (width / 2);

		//        /\           /\
		//       /  \         /__\       ____   SMALL TRIANGLE
		//      /    \  -->  /\  /\  --> \  /   bounding box: l, t, r, b (, cX)
		//     /______\     /__\/__\      \/
		w = width / 2;
		h = height / 2;
		l = (left + centerX) / 2;
		t = top + h;
		r = l + w;
		//int b = bottom;
		//int cx = centerX;

		m_tmw->w_triangle()->recalculatePositions(w, h, BarycentricTriangle(l, t, r, t, centerX, bottom));
		m_triangleBigWidth = width;
	}

	// Set up the histograms' transforms and resize them
	{
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
		m_tmw->w_histogram(0)->resize(size);
		m_tmw->w_histogram(1)->resize(size);
		m_tmw->w_histogram(2)->resize(size);
	}

	// Set up the slicers's transforms and resize them
	{
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
		m_slicerClipPaths[2].translate(w / 2, -h);

		m_slicerClipPaths[0] = m_slicerClipPaths[1]; // right
		m_slicerClipPaths[0].translate(w, 0);

		QSize size = QSize(w, h);
		m_tmw->w_slicer(0)->getSlicer()->widget()->resize(size);
		m_tmw->w_slicer(1)->getSlicer()->widget()->resize(size);
		m_tmw->w_slicer(2)->getSlicer()->widget()->resize(size);

		qDebug() << "Slicers resized to" << size.width() << "x" << size.height();
		
	}

	int histoLateral1_2Y = bottom - TRIANGLE_TOP;
	int histoTop1X = centerX - TRIANGLE_LEFT;
	int histoTop2X = centerX + TRIANGLE_LEFT;

	// Set up the clip path covering all widgets
	{
		m_clipPath = QPainterPath();
		m_clipPath.moveTo(left, bottom);
		m_clipPath.lineTo(boxLeft, histoLateral1_2Y);
		m_clipPath.lineTo(histoTop1X, boxTop);
		m_clipPath.lineTo(centerX, top);
		m_clipPath.lineTo(histoTop2X, boxTop);
		m_clipPath.lineTo(boxRight, histoLateral1_2Y);
		m_clipPath.lineTo(right, bottom);
		m_clipPath.lineTo(right, boxBottom);
		m_clipPath.lineTo(left, boxBottom);
		m_clipPath.lineTo(left, bottom);
	}

	// Other widgets
	{
		int slicerModeLabelWidth = m_tmw->w_slicerModeLabel()->sizeHint().width();
		int layoutTypeComboBoxWidth = m_tmw->w_layoutComboBox()->sizeHint().width();
		int sliceNumberLabelWidth = m_tmw->w_sliceNumberLabel()->sizeHint().width();
		int checkBoxWeightByOpacityWidth = m_tmw->w_checkBox_weightByOpacity()->sizeHint().width();

		int controlsWidth = qMax(qMax(slicerModeLabelWidth, qMax(layoutTypeComboBoxWidth, checkBoxWeightByOpacityWidth)), sliceNumberLabelWidth);
		int controlsBottom = controlsWidth * (-histoLateral1_2Y / histoTop1X) + histoLateral1_2Y;

		int slicerModeComboBoxHeight = m_tmw->w_slicerModeLabel()->sizeHint().height();
		int checkBoxWeightByOpacityHeight = m_tmw->w_checkBox_weightByOpacity()->sizeHint().height();
		int layoutTypeComboBoxHeight = m_tmw->w_layoutComboBox()->sizeHint().height();
		int sliderHeight = controlsBottom - slicerModeComboBoxHeight - layoutTypeComboBoxHeight - checkBoxWeightByOpacityHeight;

		int bottom = 0;

		m_tmw->w_layoutComboBox()->setGeometry(QRect(0, bottom, controlsWidth, layoutTypeComboBoxHeight));
		bottom += layoutTypeComboBoxHeight;

		m_tmw->w_checkBox_weightByOpacity()->setGeometry(QRect(0, bottom, controlsWidth, checkBoxWeightByOpacityHeight));
		bottom += checkBoxWeightByOpacityHeight;

		m_tmw->w_slicerModeLabel()->setGeometry(QRect(0, bottom, controlsWidth, layoutTypeComboBoxHeight));
		bottom += slicerModeComboBoxHeight;

		m_tmw->w_sliceNumberLabel()->setGeometry(QRect(0, bottom, controlsWidth, sliderHeight));

		m_rControls = QRect(0, 0, controlsWidth, controlsBottom);
	}

	// Triangle's labels and weights
	{
		int tangent1_2Y = (boxTop + histoLateral1_2Y) / 2;
		QPoint textPoint1 = QPoint((boxLeft + histoTop1X) / 2, tangent1_2Y);
		QPoint textPoint2 = QPoint((histoTop2X + boxRight) / 2, tangent1_2Y);

		QRect weightRects[3] = {
			m_tmw->w_triangle()->getModalityWeightRect(0),
			m_tmw->w_triangle()->getModalityWeightRect(1),
			m_tmw->w_triangle()->getModalityWeightRect(2)
		};
		QRect labelRects[3] = {
			m_tmw->w_triangle()->getModalityLabelRect(0),
			m_tmw->w_triangle()->getModalityLabelRect(1),
			m_tmw->w_triangle()->getModalityLabelRect(2)
		};
		
		/*QPoint weightPoints[3] = {
			QPoint(textPoint1.x() - weightRects[0].width(), textPoint1.y()),
			textPoint2,
			QPoint(right, ((bottom + boxBottom) / 2) + ((labelRects[2].height() + weightRects[2].height()) / 2))
		};
		QPoint labelPoints[3] = {
			QPoint(textPoint1.x() - labelRects[0].width(), textPoint1.y() - weightRects[0].height()),
			QPoint(textPoint2.x(), textPoint2.y() - weightRects[1].height()),
			QPoint(weightPoints[2].x(), weightPoints[2].y() - weightRects[2].height())
		};*/

		QPoint labelPoints[3] = {
			textPoint1 + QPoint(-labelRects[0].width() - MODALITY_LABEL_MARGIN, 0),
			textPoint2 + QPoint(MODALITY_LABEL_MARGIN, 0),
			QPoint(right + MODALITY_LABEL_MARGIN, ((bottom + boxBottom) / 2) - (labelRects[2].height() / 2))
		};
		QPoint weightPoints[3] = {
			labelPoints[0] + QPoint(-MODALITY_LABEL_MARGIN - weightRects[0].width(), 0),
			labelPoints[1] + QPoint( MODALITY_LABEL_MARGIN + labelRects[1].width(), 0),
			labelPoints[2] + QPoint( MODALITY_LABEL_MARGIN + labelRects[2].width(), 0)
		};

		m_tmw->w_triangle()->setModalityWeightPosition(weightPoints[0], 0);
		m_tmw->w_triangle()->setModalityWeightPosition(weightPoints[1], 1);
		m_tmw->w_triangle()->setModalityWeightPosition(weightPoints[2], 2);
			   
		m_tmw->w_triangle()->setModalityLabelPosition(labelPoints[0], 0);
		m_tmw->w_triangle()->setModalityLabelPosition(labelPoints[1], 1);
		m_tmw->w_triangle()->setModalityLabelPosition(labelPoints[2], 2);
	}

	m_fClear = true;
}

void iAHistogramTriangle::paintEvent(QPaintEvent* event)
{
	// Unfortunatelly, enabling this mechanism of not rendering everything leads to
	// many updates not occurring. Probably a consequence of the Qt hacks this class
	// does... No idea how to fix that, so disable this system for now (=> m_fClear = true).
	m_fClear = true;
	if (m_fClear) {
		m_fRenderHistogram[0] = true;
		m_fRenderHistogram[1] = true;
		m_fRenderHistogram[2] = true;
		m_fRenderSlicer[0] = true;
		m_fRenderSlicer[1] = true;
		m_fRenderSlicer[2] = true;
		m_fRenderTriangle = true;

		m_buffer = QImage(size().width(), size().height(), QImage::Format::Format_RGB32);
		m_buffer.fill(Qt::white);
	}

	QPainter p;
	p.begin(&m_buffer);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	//p.setClipPath(m_clipPath);

	qDebug() << "Slice numbers (A B C):" << m_tmw->w_slicer(0)->getSliceNumber() << m_tmw->w_slicer(1)->getSliceNumber() << m_tmw->w_slicer(2)->getSliceNumber();
	paintSlicers(p);

	if (m_fRenderTriangle) {
		m_tmw->w_triangle()->paintContext(p);
	}

	paintHistograms(p);

	if (m_fRenderHistogram[0] || m_fRenderHistogram[1] || m_fRenderHistogram[2]) {
		//p.setClipping(false);
		p.setPen(m_clipPathPen);
		p.drawPath(m_clipPath);
	}

	p.end();

	p.begin(this);
	p.drawImage(0, 0, m_buffer);
	m_tmw->w_triangle()->paintModalityLabels(p); // Is repainted everytime!
	m_tmw->w_triangle()->paintControlPoint(p);
	p.end();

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

			img = m_tmw->w_slicer(i)->getSlicer()->widget()->GRAB_FRAMEBUFFER();

			QSize size = m_tmw->w_slicer(i)->getSlicer()->widget()->size();
			//qDebug() << "Slicer framebuffer valid?" << m_tmw->m_slicerWidgets[i]->getSlicer()->widget()->isValid();
			qDebug() << "Slicer size:" << size.width() << "x" << size.height();
			qDebug() << "Image size:" << img.size().width() << "x" << img.size().height();

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

	// LEFT, RIGHT then BOTTOM
	for (int i = 0; i < 3; i++) {
		if (m_fRenderHistogram[i]) {
			p.setTransform(m_transformHistograms[i]);
			auto img = m_tmw->w_histogram(i)->drawOffscreen();
			p.drawImage(0, 0, img);
			//p.drawRect(m_histogramsRect);
		}
	}

	p.resetTransform();
	//p.setPen(oldPen);
}