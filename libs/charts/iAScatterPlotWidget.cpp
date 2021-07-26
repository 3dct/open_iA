/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAScatterPlotWidget.h"

#include "iALog.h"
#include "iALookupTable.h"
#include "iAScatterPlot.h"
#include "iAScatterPlotViewData.h"
#include "iASPLOMData.h"

#include <vtkLookupTable.h>
#include <vtkSmartPointer.h>

#include <QApplication>    // for qApp->palette()

#include <QMouseEvent>
#if (defined(CHART_OPENGL) && defined(OPENGL_DEBUG))
#include <QOpenGLDebugLogger>
#endif
#include <QPainter>


namespace
{
	const int PaddingLeftBase = 2;
	const int PaddingBottomBase = 2;
}
const int iAScatterPlotWidget::PaddingTop = 5;
const int iAScatterPlotWidget::PaddingRight = 5;
const int iAScatterPlotWidget::TextPadding = 5;


iAScatterPlotWidget::iAScatterPlotWidget(QSharedPointer<iASPLOMData> data) :
	m_data(data),
	m_viewData(new iAScatterPlotViewData()),
	m_fontHeight(0),
	m_maxTickLabelWidth(0)
{
#ifdef CHART_OPENGL
	auto fmt = defaultQOpenGLWidgetFormat();
	#ifdef SP_OLDOPENGL
	fmt.setStereo(true);
	#endif
	setFormat(fmt);
#endif
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	m_scatterplot = new iAScatterPlot(m_viewData.data(), this);
	m_scatterplot->settings.selectionEnabled = true;
	data->updateRanges();
	if (data->numPoints() > std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("Number of points (%1) larger than supported (%2)")
			.arg(data->numPoints())
			.arg(std::numeric_limits<int>::max()));
	}
	m_scatterplot->setData(0, 1, data);
	connect(m_viewData.data(), &iAScatterPlotViewData::updateRequired, this, QOverload<>::of(&iAChartParentWidget::update));
	connect(m_scatterplot, &iAScatterPlot::currentPointModified, this, &iAScatterPlotWidget::currentPointUpdated);
}

void iAScatterPlotWidget::currentPointUpdated(size_t index)
{
	Q_UNUSED(index);
	m_viewData->updateAnimation(m_scatterplot->getCurrentPoint(), m_scatterplot->getPreviousIndex());
}

void iAScatterPlotWidget::SetPlotColor(QColor const & c, double rangeMin, double rangeMax)
{
	auto lut = QSharedPointer<iALookupTable>::create();
	double lutRange[2] = { rangeMin, rangeMax };
	lut->setRange(lutRange);
	lut->allocate(2);
	for (int i = 0; i < 2; ++i)
	{
		lut->setColor(i, c);
	}
	m_scatterplot->setLookupTable(lut, 0);
}

#ifdef CHART_OPENGL
void iAScatterPlotWidget::paintGL()
#else
void iAScatterPlotWidget::paintEvent(QPaintEvent* /*event*/)
#endif
{
#if (defined(CHART_OPENGL) && defined(OPENGL_DEBUG))
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	assert(ctx);
	QOpenGLDebugLogger logger(this);
	logger.initialize();  // initializes in the current context, i.e. ctx
	connect(&logger, &QOpenGLDebugLogger::messageLogged,
		[](const QOpenGLDebugMessage& dbgMsg) { LOG(lvlDebug, dbgMsg.message()); });
	logger.startLogging();
#endif
	QPainter painter(this);
	QFontMetrics fm = painter.fontMetrics();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	if (m_fontHeight != fm.height() || m_maxTickLabelWidth != fm.horizontalAdvance("0.99"))
	{
		m_fontHeight = fm.height();
		m_maxTickLabelWidth = fm.horizontalAdvance("0.99");
#else
	if (m_fontHeight != fm.height() || m_maxTickLabelWidth != fm.width("0.99"))
	{
		m_fontHeight = fm.height();
		m_maxTickLabelWidth = fm.width("0.99");
#endif
	}
	painter.setRenderHint(QPainter::Antialiasing);
	QColor bgColor(qApp->palette().color(QWidget::backgroundRole()));
	QColor fg(qApp->palette().color(QPalette::Text));
	m_scatterplot->settings.tickLabelColor = fg;
#if (defined(CHART_OPENGL))
	painter.beginNativePainting();
	glClearColor(bgColor.red() / 255.0, bgColor.green() / 255.0, bgColor.blue() / 255.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	painter.endNativePainting();
#else
	painter.fillRect(rect(), bgColor);
#endif
	m_scatterplot->paintOnParent(painter);

	// print axes tick labels:
	painter.save();
	QList<double> ticksX, ticksY; QList<QString> textX, textY;
	m_scatterplot->printTicksInfo(&ticksX, &ticksY, &textX, &textY);
	painter.setPen(m_scatterplot->settings.tickLabelColor);
	QPoint tOfs(PaddingLeft(), PaddingBottom());
	long tSpc = 5;
	for (long i = 0; i < ticksY.size(); ++i)
	{
		double t = ticksY[i]; QString text = textY[i];
		painter.drawText(QRectF(0, t - tOfs.y(), tOfs.x() - tSpc, 2 * tOfs.y()), Qt::AlignRight | Qt::AlignVCenter, text);
	}
	painter.rotate(-90);
	for (long i = 0; i < ticksX.size(); ++i)
	{
		double t = ticksX[i]; QString text = textX[i];
		painter.drawText(QRectF(-tOfs.y() + tSpc + PaddingBottom() - height() - TextPadding,
				t - tOfs.x(), tOfs.y() - tSpc, 2 * tOfs.x()), Qt::AlignRight | Qt::AlignVCenter, text);
	}
	painter.restore();

	// print axes labels:
	painter.save();
	painter.setPen(m_scatterplot->settings.tickLabelColor);
	painter.drawText(QRectF(0, height() - fm.height() - TextPadding, width(), fm.height()),
			Qt::AlignHCenter | Qt::AlignTop, m_data->parameterName(0));
	painter.rotate(-90);
	painter.drawText(QRectF(-height(), 0, height(), fm.height()), Qt::AlignCenter | Qt::AlignTop, m_data->parameterName(1));
	painter.restore();
}

void iAScatterPlotWidget::adjustScatterPlotSize()
{
	QRect size(geometry());
	size.moveTop(0);
	size.moveLeft(0);
	size.adjust(PaddingLeft(), PaddingTop, -PaddingRight, -PaddingBottom());
	if (size.width() > 0 && size.height() > 0)
	{
		m_scatterplot->setRect(size);
	}
}

void iAScatterPlotWidget::resizeEvent(QResizeEvent* event)
{
	adjustScatterPlotSize();
	iAChartParentWidget::resizeEvent(event);
}

int iAScatterPlotWidget::PaddingLeft()
{
	return PaddingLeftBase+m_fontHeight+m_maxTickLabelWidth+TextPadding;
}

int iAScatterPlotWidget::PaddingBottom()
{
	return PaddingBottomBase+m_fontHeight+m_maxTickLabelWidth+TextPadding;
}

void iAScatterPlotWidget::wheelEvent(QWheelEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	if (event->x() >= PaddingLeft() && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom()))
#else
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
#endif
	{
		m_scatterplot->SPLOMWheelEvent(event);
		update();
	}
}

void iAScatterPlotWidget::mousePressEvent(QMouseEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	if (event->x() >= PaddingLeft() && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom()))
#else
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
#endif
	{
		m_scatterplot->SPLOMMousePressEvent(event);
	}
}

void iAScatterPlotWidget::mouseReleaseEvent(QMouseEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	if (event->x() >= PaddingLeft() && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom()))
#else
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
#endif
	{
		m_scatterplot->SPLOMMouseReleaseEvent(event);
		update();
	}
}

void iAScatterPlotWidget::mouseMoveEvent(QMouseEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	if (event->x() >= PaddingLeft() && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom()))
#else
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
#endif
	{
		m_scatterplot->SPLOMMouseMoveEvent(event);
	}
}

void iAScatterPlotWidget::keyPressEvent(QKeyEvent * event)
{
	switch (event->key())
	{
	case Qt::Key_R: //if R is pressed, reset all the applied transformation as offset and scaling
		m_scatterplot->setTransform(1.0, QPointF(0.0f, 0.0f));
		break;
	}
}

void iAScatterPlotWidget::SetSelectionColor(QColor const & c)
{
	m_scatterplot->settings.selectionColor = c;
}

void iAScatterPlotWidget::SetSelectionMode(iAScatterPlot::SelectionMode mode)
{
	m_scatterplot->settings.selectionMode = mode;
}

QSharedPointer<iAScatterPlotViewData> iAScatterPlotWidget::viewData()
{
	return m_viewData;
}
