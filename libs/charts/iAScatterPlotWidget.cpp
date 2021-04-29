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

#include <QActionGroup>
#include <QMenu>
#include <QMouseEvent>
#if (defined(CHART_OPENGL) && defined(OPENGL_DEBUG))
#include <QOpenGLDebugLogger>
#endif
#include <QPainter>

// for popup / tooltip:
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>

class iADefaultScatterPlotPointInfo : public iAScatterPlotPointInfo
{
public:
	iADefaultScatterPlotPointInfo(QSharedPointer<iASPLOMData> data) :
		m_data(data)
	{}
	QString text(const size_t paramIdx[2], size_t pointIdx) override
	{
		return m_data->parameterName(paramIdx[0]) + ": " +
			QString::number(m_data->paramData(paramIdx[0])[pointIdx]) + "<br>" +
			m_data->parameterName(paramIdx[1]) + ": " +
			QString::number(m_data->paramData(paramIdx[1])[pointIdx]);
	}
private:
	QSharedPointer<iASPLOMData> m_data;
};

namespace
{
	const int PaddingLeftBase = 2;
	const int PaddingBottomBase = 2;
}
const int iAScatterPlotWidget::PaddingTop = 5;
const int iAScatterPlotWidget::PaddingRight = 5;
const int iAScatterPlotWidget::TextPadding = 5;


iAScatterPlotWidget::iAScatterPlotWidget(QSharedPointer<iASPLOMData> data, bool columnSelection) :
	m_data(data),
	m_viewData(new iAScatterPlotViewData()),
	m_fontHeight(0),
	m_maxTickLabelWidth(0),
	m_fixPointsEnabled(false),
	m_pointInfo(new iADefaultScatterPlotPointInfo(data)),
	m_contextMenu(nullptr)
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
	if (columnSelection)
	{
		m_contextMenu = new QMenu(this);
		auto xMenu = m_contextMenu->addMenu("X Parameter");
		auto yMenu = m_contextMenu->addMenu("Y Parameter");
		auto xActGrp = new QActionGroup(m_contextMenu);
		auto yActGrp = new QActionGroup(m_contextMenu);
		for (size_t p = 0; p < data->numParams(); ++p)
		{
			auto xShowAct = new QAction(data->parameterName(p), this);
			xShowAct->setCheckable(true);
			xShowAct->setChecked(p == 0);
			xShowAct->setActionGroup(xActGrp);
			xShowAct->setProperty("idx", static_cast<unsigned long long>(p));
			connect(xShowAct, &QAction::triggered, this, &iAScatterPlotWidget::xParamChanged);
			xMenu->addAction(xShowAct);

			auto yShowAct = new QAction(data->parameterName(p), this);
			yShowAct->setCheckable(true);
			yShowAct->setChecked(p == 1);
			yShowAct->setActionGroup(yActGrp);
			yShowAct->setProperty("idx", static_cast<unsigned long long>(p));
			connect(yShowAct, &QAction::triggered, this, &iAScatterPlotWidget::yParamChanged);
			yMenu->addAction(yShowAct);
		}
	}
	m_scatterplot->setData(0, 1, data);
	connect(m_viewData.data(), &iAScatterPlotViewData::updateRequired, this, QOverload<>::of(&iAChartParentWidget::update));
	connect(m_scatterplot, &iAScatterPlot::currentPointModified, this, &iAScatterPlotWidget::currentPointUpdated);
	connect(m_scatterplot, &iAScatterPlot::selectionModified, this, &iAScatterPlotWidget::selectionModified);
}

void iAScatterPlotWidget::currentPointUpdated(size_t index)
{
	Q_UNUSED(index);
	m_viewData->updateAnimation(m_scatterplot->getCurrentPoint(), m_scatterplot->getPreviousIndex());
}

void iAScatterPlotWidget::setSelectionEnabled(bool enabled)
{
	m_scatterplot->settings.selectionEnabled = enabled;
}

QSharedPointer<iAScatterPlotViewData> iAScatterPlotWidget::viewData()
{
	return m_viewData;
}

void iAScatterPlotWidget::xParamChanged()
{
	size_t idx = sender()->property("idx").toULongLong();
	m_scatterplot->setIndices(idx, m_scatterplot->getIndices()[1]);
	update();
}

void iAScatterPlotWidget::yParamChanged()
{
	size_t idx = sender()->property("idx").toULongLong();
	m_scatterplot->setIndices(m_scatterplot->getIndices()[0], idx);
	update();
}

void iAScatterPlotWidget::setPlotColor(QColor const & c, double rangeMin, double rangeMax)
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

void iAScatterPlotWidget::setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIdx)
{
	m_scatterplot->setLookupTable(lut, paramIdx);
	update();
}

QSharedPointer<iALookupTable> iAScatterPlotWidget::lookupTable() const
{
	return m_scatterplot->lookupTable();
}

#ifdef CHART_OPENGL
void iAScatterPlotWidget::paintGL()
#else
void iAScatterPlotWidget::paintEvent(QPaintEvent* event)
#endif
{
#if (defined(CHART_OPENGL) && defined(OPENGL_DEBUG))
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(this);
	logger->initialize();  // initializes in the current context, i.e. ctx
	connect(logger, &QOpenGLDebugLogger::messageLogged,
		[](const QOpenGLDebugMessage& dbgMsg) { LOG(lvlDebug, dbgMsg.message()); });
	logger->startLogging();
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
	Q_UNUSED(event);
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
		QRectF textRect(0, t - tOfs.y(), tOfs.x() - tSpc, 2 * tOfs.y());
		//LOG(lvlInfo, QString("text rect: %1,%2, %3x%4").arg(textRect.left()).arg(textRect.top()).arg(textRect.width()).arg(textRect.height()));
		painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);
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
	painter.drawText(QRectF(-PaddingLeft(), height() - fm.height() - TextPadding, width(), fm.height()),
			Qt::AlignHCenter | Qt::AlignTop, m_data->parameterName(m_scatterplot->getIndices()[0]));
	painter.rotate(-90);
	painter.drawText(QRectF(-height(), 0, height(), fm.height()), Qt::AlignCenter | Qt::AlignTop, m_data->parameterName(m_scatterplot->getIndices()[1]));
	painter.restore();

	drawTooltip(painter);
}

void iAScatterPlotWidget::drawTooltip(QPainter& painter)
{
	size_t curInd = m_scatterplot->getCurrentPoint();
	if (curInd == iASPLOMData::NoDataIdx)
	{
		return;
	}
	const size_t* pInds = m_scatterplot->getIndices();

	painter.save();
	QPointF popupPos = m_scatterplot->getPointPosition(curInd);
	double pPM = m_scatterplot->settings.pickedPointMagnification;
	double ptRad = m_scatterplot->getPointRadius();
	popupPos.setY(popupPos.y() - pPM * ptRad);
	QColor popupFillColor(qApp->palette().color(QPalette::Window));
	painter.setBrush(popupFillColor);
	QColor popupBorderColor(qApp->palette().color(QPalette::Dark));
	painter.setPen(popupBorderColor);
	painter.translate(popupPos);
	QString text = "<b>#" + QString::number(curInd) + "</b><br> " +
		m_pointInfo->text(pInds, curInd);
	QTextDocument doc;
	doc.setHtml(text);
	int popupWidth = 200;	// = settings.popupWidth
	doc.setTextWidth(popupWidth);
	double tipDim[2] = { 5, 10 }; // = settings.popupTipDim
	double popupWidthHalf = popupWidth / 2; // settings.popupWidth / 2
	auto popupHeight = doc.size().height();
	QPointF points[7] = {
		QPointF(0, 0),
		QPointF(-tipDim[0], -tipDim[1]),
		QPointF(-popupWidthHalf, -tipDim[1]),
		QPointF(-popupWidthHalf, -popupHeight - tipDim[1]),
		QPointF(popupWidthHalf, -popupHeight - tipDim[1]),
		QPointF(popupWidthHalf, -tipDim[1]),
		QPointF(tipDim[0], -tipDim[1]),
	};
	painter.drawPolygon(points, 7);

	painter.translate(-popupWidthHalf, -popupHeight - tipDim[1]);
	QAbstractTextDocumentLayout::PaintContext ctx;
	QColor popupTextColor(qApp->palette().color(QPalette::ToolTipText));  // = settings.popupTextColor;
	ctx.palette.setColor(QPalette::Text, popupTextColor);
	doc.documentLayout()->draw(&painter, ctx); //doc.drawContents( &painter );
	painter.restore();
}

void iAScatterPlotWidget::adjustScatterPlotSize()
{
	QRect size(geometry());
	size.moveTop(0);
	size.moveLeft(0);
	size.adjust(PaddingLeft(), PaddingTop, -PaddingRight, -PaddingBottom());
	//LOG(lvlInfo, QString("%1,%2 %3x%4").arg(size.top()).arg(size.left()).arg(size.width()).arg(size.height()));
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
	if (m_fixPointsEnabled)
	{
		toggleHighlightedPoint(m_scatterplot->getCurrentPoint(), event->modifiers());
	}
}

void iAScatterPlotWidget::toggleHighlightedPoint(size_t curPoint, Qt::KeyboardModifiers modifiers)
{
	if (!modifiers.testFlag(Qt::ControlModifier))
	{  // if Ctrl key not pressed, deselect all highlighted points on any click
		for (auto idx : m_viewData->highlightedPoints())
		{
			emit pointHighlighted(idx, false);
		}
		m_viewData->clearHighlightedPoints();
	}
	if (curPoint != iASPLOMData::NoDataIdx)
	{
		auto wasHighlighted = m_viewData->isPointHighlighted(curPoint);
		if (modifiers.testFlag(Qt::ControlModifier) && wasHighlighted)
		{  // remove just the highlight of current point if Ctrl _is_ pressed
			m_viewData->removeHighlightedPoint(curPoint);
			emit pointHighlighted(curPoint, false);
		}
		else if (!wasHighlighted)
		{  // if current point was not highlighted before, add it
			m_viewData->addHighlightedPoint(curPoint);
			emit pointHighlighted(curPoint, true);
		}
	}
	emit highlightChanged();
}

void iAScatterPlotWidget::setHighlightColor(QColor hltCol)
{
	m_scatterplot->setHighlightColor(hltCol);
}

void iAScatterPlotWidget::setHighlightDrawMode(iAScatterPlot::HighlightDrawModes drawMode)
{
	m_scatterplot->setHighlightDrawMode(drawMode);
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
	if (event->key() == Qt::Key_R) //if R is pressed, reset all the applied transformation as offset and scaling
	{
		m_scatterplot->setTransform(1.0, QPointF(0.0f, 0.0f));
	}
}

void iAScatterPlotWidget::contextMenuEvent(QContextMenuEvent* event)
{
	if (m_contextMenu)
	{
		m_contextMenu->exec(event->globalPos());
	}
}
void iAScatterPlotWidget::setSelectionColor(QColor const & c)
{
	m_scatterplot->settings.selectionColor = c;
}

void iAScatterPlotWidget::setSelectionMode(iAScatterPlot::SelectionMode mode)
{
	m_scatterplot->settings.selectionMode = mode;
}

void iAScatterPlotWidget::setPickedPointFactor(double factor)
{
	m_scatterplot->settings.pickedPointMagnification = factor;
}

void iAScatterPlotWidget::setPointRadius(double pointRadius)
{
	m_scatterplot->setPointRadius(pointRadius);
}

void iAScatterPlotWidget::setFixPointsEnabled(bool enabled)
{
	m_fixPointsEnabled = enabled;
}

void iAScatterPlotWidget::setPointInfo(QSharedPointer<iAScatterPlotPointInfo> pointInfo)
{
	m_pointInfo = pointInfo;
}

