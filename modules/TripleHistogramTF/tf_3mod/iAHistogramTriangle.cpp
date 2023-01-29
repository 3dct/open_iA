// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramTriangle.h"

#include "iABarycentricTriangleWidget.h"
#include "iATripleModalityWidget.h"
#include "iASimpleSlicerWidget.h"

#include <iAChartWithFunctionsWidget.h>
#include <iASlicer.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QResizeEvent>
#include <QSlider>
#include <QtMath>

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
const static int WIDGETS_MARGIN = 10;

iAHistogramTriangle::iAHistogramTriangle(iATripleModalityWidget* tripleModalityWidget)
	: m_tmw(tripleModalityWidget)
{
}

void iAHistogramTriangle::glresized()
{
	qDebug() << "GL RESIZED!";
}

void iAHistogramTriangle::initialize(std::array<QString, 3> names)
{
	m_clipPathPen.setWidth(1);
	m_clipPathPen.setColor(Qt::black);

	m_slicerBackgroundBrush.setColor(Qt::black);

	setMouseTracking(true);

	QWidget *widget = new QWidget(this);
	QLayout *layout = new QHBoxLayout(widget);
	for (int i = 0; i < 3; i++)
	{
		layout->addWidget(m_tmw->w_slicer(i)->getSlicer());

		// The following call will make sure that
		// m_tmw->m_slicerWidgets[i]->getSlicer()->widget()->isValid()
		// returns true (see isValid() definition 19/04/2019)
		// That prevents a "Framebuffer incomplete attachment" warning
#ifdef CHART_OPENGL
// TODO: Find way to do this without OpenGL!
		m_tmw->w_slicer(i)->getSlicer()->grabFramebuffer();
#endif
	}

	//QLayout *thisLayout = new QHBoxLayout(this);
	//thisLayout->addWidget(temp);

	m_tmw->w_layoutComboBox()->setParent(this);
	m_tmw->w_sliceNumberLabel()->setParent(this);
	m_tmw->w_slicerModeLabel()->setParent(this);
	m_tmw->w_checkBox_weightByOpacity()->setParent(this);
	m_tmw->w_checkBox_syncedCamera()->setParent(this);

	calculatePositions();
	m_fClear = true;
	update();

	for (int i = 0; i < 3; i++)
	{
		connect(m_tmw->w_slicer(i)->getSlicer(), &iASlicer::resized, this, &iAHistogramTriangle::glresized);
	}
	connect(m_tmw, &iATripleModalityWidget::slicerModeChangedExternally, this, &iAHistogramTriangle::updateSlicers);
	connect(m_tmw, &iATripleModalityWidget::sliceNumberChangedExternally, this, &iAHistogramTriangle::updateSlicers);
}

void iAHistogramTriangle::resizeEvent(QResizeEvent* event)
{
	calculatePositions(event->size().width(), event->size().height());
}

void iAHistogramTriangle::updateSlicers()
{
	for (int i = 0; i < 3; i++)
	{
		m_fRenderSlicer[i] = true;
	}
	update();
}

void iAHistogramTriangle::updateHistograms()
{
	for (int i = 0; i < 3; i++)
	{
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
	
	// TODO NEWIO: create copy of event with adapted pos
	//QMouseEvent* newEvent = new QMouseEvent(event->type(), event->localPos(), event->button(), event->buttons(), event->modifiers(), event->source());
	if (m_draggedType == HISTOGRAM)
	{
		transformed = m_transformHistograms[m_lastIndex].inverted().map(event->pos());
	} else if (m_draggedType == SLICER)
	{
		transformed = m_transformSlicers[m_lastIndex].inverted().map(event->pos());
	}

	if (m_draggedType == TRIANGLE || (m_draggedType == NONE && onTriangle(event->pos())))
	{
		m_fRenderTriangle = true;

		widgetType = TRIANGLE;
		target = m_tmw->w_triangle();
	}

	else if (m_draggedType == HISTOGRAM || (m_draggedType == NONE && (target = onHistogram(event->pos(), transformed))))
	{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
		event->setLocalPos(transformed);
#else
		// TODO: setLocalPos was removed (was undocumented before)
#endif
		m_fRenderHistogram[m_lastIndex] = true; // m_lastIndex is affected by onHistogram function
		m_fRenderSlicer[m_lastIndex] = true; // m_lastIndex is affected by onHistogram function

		widgetType = HISTOGRAM;
		target = target ? target : m_draggedWidget;
	}

	else if (m_draggedType == SLICER || (m_draggedType == NONE && (target = onSlicer(event->pos(), transformed))))
	{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
		event->setLocalPos(transformed);
#else
		// TODO: setLocalPos was removed (was undocumented before)
#endif
		m_fRenderSlicer[m_lastIndex] = true; // m_lastIndex is affected by onHistogram function

		widgetType = SLICER;
		target = target ? target : m_draggedWidget;
	}
	else
	{
		// the triangle widget actually occupies the whole screen, despite the triangle
		//     itself being smaller
		// the modality labels, for example, are part of the triangle widget
		// to keep the interaction, forward the event to the triangle widget here
		target = m_tmw->w_triangle();

		// BUT DON'T RENDER THE TRIANGLE!
		// The modality labels are rendered anyway, there's no need for this flag to be set
		//m_fRenderTriangle = true;
	}

	if (eventType == PRESS)
	{
		m_draggedType = widgetType;
		m_draggedWidget = target;
	}
	else if (eventType == RELEASE)
	{
		m_draggedType = NONE;
		m_draggedWidget = nullptr;
	}

	QApplication::sendEvent(target, event);
	update();
}

void iAHistogramTriangle::forwardWheelEvent(QWheelEvent *e)
{
	QPoint transformed;
	iASlicer* target = onSlicer(e->position().toPoint(), transformed);
	if (!target)
	{
		return;
	}
	QWheelEvent* newE = new QWheelEvent(transformed, e->globalPosition(), e->pixelDelta(), e->angleDelta(),
		e->buttons(), e->modifiers(), e->phase(), e->inverted(), e->source());
	QApplication::sendEvent(target, newE);
	m_fRenderSlicer[m_lastIndex] = true;
	update();
}

void iAHistogramTriangle::forwardContextMenuEvent(QContextMenuEvent *e)
{
	QPoint transformed;
	iAChartWithFunctionsWidget *target = onHistogram(e->pos(), transformed);
	if (!target)
	{
		return;
	}
	QContextMenuEvent *newE = new QContextMenuEvent(e->reason(), transformed, e->globalPos());
	QApplication::sendEvent(target, newE);
	//m_fRenderHistogram[index] = true;
	//update();
}

iAChartWithFunctionsWidget* iAHistogramTriangle::onHistogram(QPoint p, QPoint &transformed)
{
	for (int i = 0; i < 3; i++)
	{
		transformed = m_transformHistograms[i].inverted().map(p);
		if (m_histogramsRect.contains(transformed))
		{
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

iASlicer* iAHistogramTriangle::onSlicer(QPoint p, QPoint &transformed)
{
	for (int i = 0; i < 3; i++)
	{
		if (m_slicerTriangles[i].contains(p.x(), p.y()))
		{
			transformed = m_transformSlicers[i].inverted().map(p);
			m_lastIndex = i;
			return m_tmw->w_slicer(i)->getSlicer();
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

		if ((double)aw / (double)ah < TRIANGLE_WIDTH_RATIO)
		{
			width = aw;
			height = qRound(aw * TRIANGLE_HEIGHT_RATIO);
		}
		else
		{
			width = qRound(ah * TRIANGLE_WIDTH_RATIO);
			height = ah;
		}

		// The box will bound all the elements (triangle, histograms, slices)
		boxLeft = (aw - width) / 2;
		boxTop = (ah - height) / 2;
		boxRight = boxLeft + width + TRIANGLE_LEFT + TRIANGLE_RIGHT;
		boxBottom = boxTop + height + TRIANGLE_TOP + TRIANGLE_BOTTOM;

		// NOTE: CAREFUL with "\" -> it is the "line continuation" character
		// -> if used at end of a single line comment line, it makes next line also a comment!
		//        /\     |
		//       /  \    | BIG TRIANGLE
		//      /    \   | bounding box:
		//     /______\  | left, top, right, bottom (, centerX)
		left = boxLeft + TRIANGLE_LEFT;
		top = boxTop + TRIANGLE_TOP;
		right = left + width;
		bottom = top + height;
		centerX = left + (width / 2);

		//        /\           /\               |
		//       /  \         /__\       ____   | SMALL TRIANGLE
		//      /    \  -->  /\  /\  --> \  /   | bounding box: l, t, r, b (, cX)
		//     /______\     /__\/__\      \/    |
		w = width / 2;
		h = height / 2;
		l = (left + centerX) / 2;
		t = top + h;
		r = l + w;
		//int b = bottom;
		//int cx = centerX;

		m_tmw->w_triangle()->recalculatePositions(w, h, iABarycentricTriangle(l, t, r, t, centerX, bottom));
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
		m_tmw->w_slicer(0)->getSlicer()->resize(size);
		m_tmw->w_slicer(1)->getSlicer()->resize(size);
		m_tmw->w_slicer(2)->getSlicer()->resize(size);
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
		int layoutTypeComboBoxWidth = m_tmw->w_layoutComboBox()->sizeHint().width();
		int checkBoxWeightByOpacityWidth = m_tmw->w_checkBox_weightByOpacity()->sizeHint().width();
		int checkBoxSyncCameraWidth = m_tmw->w_checkBox_syncedCamera()->sizeHint().width();
		int slicerModeLabelWidth = m_tmw->w_slicerModeLabel()->sizeHint().width();
		int sliceNumberLabelWidth = m_tmw->w_sliceNumberLabel()->sizeHint().width();

		int controlsWidth = qMax(qMax(qMax(qMax(slicerModeLabelWidth, layoutTypeComboBoxWidth), checkBoxSyncCameraWidth), checkBoxWeightByOpacityWidth), sliceNumberLabelWidth);
		int controlsBottom = controlsWidth * (-histoLateral1_2Y / histoTop1X) + histoLateral1_2Y;

		int slicerModeComboBoxHeight = m_tmw->w_slicerModeLabel()->sizeHint().height();
		int checkBoxWeightByOpacityHeight = m_tmw->w_checkBox_weightByOpacity()->sizeHint().height();
		int checkBoxSyncCameraHeight = m_tmw->w_checkBox_syncedCamera()->sizeHint().height();
		int layoutTypeComboBoxHeight = m_tmw->w_layoutComboBox()->sizeHint().height();
		int sliceNumberLabelHeight = m_tmw->w_sliceNumberLabel()->sizeHint().height();

		bottom = 0;

		m_tmw->w_layoutComboBox()->setGeometry(QRect(0, bottom, controlsWidth, layoutTypeComboBoxHeight));
		bottom += layoutTypeComboBoxHeight + WIDGETS_MARGIN;

		m_tmw->w_checkBox_weightByOpacity()->setGeometry(QRect(0, bottom, controlsWidth, checkBoxWeightByOpacityHeight));
		bottom += checkBoxWeightByOpacityHeight + WIDGETS_MARGIN;

		m_tmw->w_checkBox_syncedCamera()->setGeometry(QRect(0, bottom, controlsWidth, checkBoxSyncCameraHeight));
		bottom += checkBoxSyncCameraHeight + WIDGETS_MARGIN;

		m_tmw->w_slicerModeLabel()->setGeometry(QRect(0, bottom, controlsWidth, layoutTypeComboBoxHeight));
		bottom += slicerModeComboBoxHeight + WIDGETS_MARGIN;

		m_tmw->w_sliceNumberLabel()->setGeometry(QRect(0, bottom, controlsWidth, sliceNumberLabelHeight));

		m_rControls = QRect(0, 0, controlsWidth, controlsBottom);
	}

	m_fClear = true;
}

void iAHistogramTriangle::paintEvent(QPaintEvent* /*event*/)
{
	// Unfortunatelly, enabling this mechanism of not rendering everything leads to
	// many updates not occurring. Probably a consequence of the Qt hacks this class
	// does... No idea how to fix that, so disable this system for now (=> m_fClear = true).
	m_fClear = true;
	if (m_fClear)
	{
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

	if (m_fRenderTriangle)
	{
		m_tmw->w_triangle()->paintContext(p);
	}

	paintSlicers(p);

	paintHistograms(p);

	if (m_fRenderHistogram[0] || m_fRenderHistogram[1] || m_fRenderHistogram[2])
	{
		//p.setClipping(false);
		p.setPen(m_clipPathPen);
		p.drawPath(m_clipPath);
	}

	p.end();

	p.begin(this);
	p.drawImage(0, 0, m_buffer);
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
	for (int i = 0; i < 3; i++)
	{
		if (m_fRenderSlicer[i])
		{
			p.setClipping(false);
			p.fillPath(m_slicerClipPaths[i], QBrush(Qt::black)); // TODO use m_slicerBackgroundBrush

			p.setClipPath(m_slicerClipPaths[i]);
			p.setTransform(m_transformSlicers[i]);
#ifdef CHART_OPENGL
// TODO: Find way to do this without OpenGL!
			img = m_tmw->w_slicer(i)->getSlicer()->grabFramebuffer();
#endif

			QSize size = m_tmw->w_slicer(i)->getSlicer()->size();
			//qDebug() << "Slicer framebuffer valid?" << m_tmw->m_slicerWidgets[i]->getSlicer()->widget()->isValid();
			qDebug() << "Slicer size:" << size.width() << "x" << size.height();
			qDebug() << "Image size:" << img.size().width() << "x" << img.size().height();
			p.drawImage(0, 0, img);

			p.resetTransform(); // otherwise, clip path in setClipPath will be transformed as well
		}
	}

	if (hasClipping)
	{
		p.setClipPath(oldClipPath);
	}
	else
	{
		p.setClipping(false);
	}
}

void iAHistogramTriangle::paintHistograms(QPainter &p)
{
	//QPen oldPen = p.pen();
	//p.setPen(m_histogramsBorderPen);

	// LEFT, RIGHT then BOTTOM
	for (int i = 0; i < 3; i++)
	{
		if (m_fRenderHistogram[i])
		{
			p.setTransform(m_transformHistograms[i]);
			auto img = m_tmw->w_histogram(i)->drawOffscreen();
			p.drawImage(0, 0, img);
			//p.drawRect(m_histogramsRect);
		}
	}

	p.resetTransform();
	//p.setPen(oldPen);
}
