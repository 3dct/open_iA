// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAScatterPlotWidget.h"

#include "iALog.h"
#include "iALookupTable.h"
#include "iAQGLWidget.h"
#include "iAScatterPlot.h"
#include "iAScatterPlotViewData.h"
#include "iASPLOMData.h"

#include <QApplication>

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


iAScatterPlotPointInfo::~iAScatterPlotPointInfo()
{
}

//! Default implementation of scatter plot point information - print x,y values and parameter names.
class iADefaultScatterPlotPointInfo : public iAScatterPlotPointInfo
{
public:
	iADefaultScatterPlotPointInfo(std::shared_ptr<iASPLOMData> data) :
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
	std::shared_ptr<iASPLOMData> m_data;
};

namespace
{
	const int PaddingLeftBase = 2;
	const int PaddingBottomBase = 2;
}
const int iAScatterPlotWidget::PaddingTop = 5;
const int iAScatterPlotWidget::PaddingRight = 5;
const int iAScatterPlotWidget::TextPadding = 5;


iAScatterPlotWidget::iAScatterPlotWidget() :
	m_viewData(new iAScatterPlotViewData())
{
	initWidget();
}

iAScatterPlotWidget::iAScatterPlotWidget(std::shared_ptr<iASPLOMData> data, bool columnSelection) :
	m_viewData(new iAScatterPlotViewData()),
	m_columnSelection(columnSelection)
{
	initWidget();
	setData(data);
}

void iAScatterPlotWidget::initWidget()
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
}

void iAScatterPlotWidget::setData(std::shared_ptr<iASPLOMData> d)
{
	m_data = d;
	m_pointInfo = std::make_shared<iADefaultScatterPlotPointInfo>(d);
	m_scatterplot = std::make_shared<iAScatterPlot>(m_viewData.get(), this, 5, false);
	m_scatterplot->settings.selectionEnabled = true;
	d->updateRanges();
	if (d->numPoints() > std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("Number of points (%1) larger than supported (%2)")
			.arg(d->numPoints())
			.arg(std::numeric_limits<int>::max()));
	}
	if (m_columnSelection)
	{
		m_contextMenu = new QMenu(this);
		m_xMenu = m_contextMenu->addMenu("X Parameter");
		m_yMenu = m_contextMenu->addMenu("Y Parameter");
		auto xActGrp = new QActionGroup(m_contextMenu);
		auto yActGrp = new QActionGroup(m_contextMenu);
		for (size_t p = 0; p < d->numParams(); ++p)
		{
			auto xShowAct = new QAction(d->parameterName(p), this);
			xShowAct->setCheckable(true);
			xShowAct->setChecked(p == 0);
			xShowAct->setActionGroup(xActGrp);
			xShowAct->setProperty("idx", static_cast<unsigned long long>(p));
			connect(xShowAct, &QAction::triggered, this, &iAScatterPlotWidget::xParamChanged);
			m_xMenu->addAction(xShowAct);

			auto yShowAct = new QAction(d->parameterName(p), this);
			yShowAct->setCheckable(true);
			yShowAct->setChecked(p == 1);
			yShowAct->setActionGroup(yActGrp);
			yShowAct->setProperty("idx", static_cast<unsigned long long>(p));
			connect(yShowAct, &QAction::triggered, this, &iAScatterPlotWidget::yParamChanged);
			m_yMenu->addAction(yShowAct);
		}
	}
	m_scatterplot->setData(0, 1, d);
	connect(m_viewData.get(), &iAScatterPlotViewData::updateRequired, this, QOverload<>::of(&iAChartParentWidget::update));
	connect(m_viewData.get(), &iAScatterPlotViewData::filterChanged, this, &iAScatterPlotWidget::updateFilter);
	connect(m_scatterplot.get(), &iAScatterPlot::currentPointModified, this, &iAScatterPlotWidget::currentPointUpdated);
	connect(m_scatterplot.get(), &iAScatterPlot::selectionModified, this, &iAScatterPlotWidget::selectionModified);
	connect(m_scatterplot.get(), &iAScatterPlot::chartClicked, this, &iAScatterPlotWidget::chartClicked);
	update();
}

iASPLOMData * iAScatterPlotWidget::data()
{
	return m_data.get();
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

void iAScatterPlotWidget::setVisibleParameters(size_t p1, size_t p2)
{
	m_scatterplot->setIndices(p1, p2);
}

void iAScatterPlotWidget::setDrawGridLines(bool enabled)
{
	m_scatterplot->settings.drawGridLines = enabled;
}

double const* iAScatterPlotWidget::yBounds() const
{
	return m_scatterplot->yBounds();
}

void iAScatterPlotWidget::setYBounds(double yMin, double yMax)
{
	m_scatterplot->setYBounds(yMin, yMax);
}

void iAScatterPlotWidget::resetYBounds()
{
	m_scatterplot->resetYBounds();
}

std::shared_ptr<iAScatterPlotViewData> iAScatterPlotWidget::viewData()
{
	return m_viewData;
}

const size_t* iAScatterPlotWidget::paramIndices() const
{
	return m_scatterplot->getIndices();
}

void iAScatterPlotWidget::xParamChanged()
{
	size_t idx = sender()->property("idx").toULongLong();
	m_scatterplot->setIndices(idx, m_scatterplot->getIndices()[1]);
	update();
	emit visibleParamChanged();
}

void iAScatterPlotWidget::yParamChanged()
{
	size_t idx = sender()->property("idx").toULongLong();
	m_scatterplot->setIndices(m_scatterplot->getIndices()[0], idx);
	update();
	emit visibleParamChanged();
}

void iAScatterPlotWidget::updateFilter()
{
	m_scatterplot->updatePoints();
	update();
}

void iAScatterPlotWidget::setPlotColor(QColor const & c, double rangeMin, double rangeMax)
{
	auto lut = std::make_shared<iALookupTable>();
	double lutRange[2] = { rangeMin, rangeMax };
	lut->setRange(lutRange);
	lut->allocate(2);
	for (int i = 0; i < 2; ++i)
	{
		lut->setColor(i, c);
	}
	m_scatterplot->setLookupTable(lut, 0);
}

void iAScatterPlotWidget::setLookupTable(std::shared_ptr<iALookupTable> lut, size_t paramIdx)
{
	m_scatterplot->setLookupTable(lut, paramIdx);
	update();
}

std::shared_ptr<iALookupTable> iAScatterPlotWidget::lookupTable() const
{
	return m_scatterplot->lookupTable();
}

#ifdef CHART_OPENGL
void iAScatterPlotWidget::initializeGL()
{
	initializeOpenGLFunctions();
}

void iAScatterPlotWidget::paintGL()
#else
void iAScatterPlotWidget::paintEvent(QPaintEvent* event)
#endif
{
#if (defined(CHART_OPENGL) && defined(OPENGL_DEBUG))
#ifndef NDEBUG
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	assert(ctx);
#endif
	QOpenGLDebugLogger logger(this);
	logger.initialize();  // initializes in the current context, i.e. ctx
	connect(&logger, &QOpenGLDebugLogger::messageLogged,
		[](const QOpenGLDebugMessage& dbgMsg) { LOG(lvlDebug, dbgMsg.message()); });
	logger.startLogging();
#endif
	QPainter painter(this);
	
	QColor bgColor(QApplication::palette().color(QWidget::backgroundRole()));
#if (defined(CHART_OPENGL))
	painter.beginNativePainting();
	glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	painter.endNativePainting();
#else
	Q_UNUSED(event);
	painter.fillRect(rect(), bgColor);
#endif

	if (!m_data)
	{
		return;
	}

	QFontMetrics fm = painter.fontMetrics();
	if (m_fontHeight != fm.height() || m_maxTickLabelWidth != fm.horizontalAdvance("-0.999"))
	{
		m_fontHeight = fm.height();
		m_maxTickLabelWidth = fm.horizontalAdvance("-0.999");
		adjustScatterPlotSize();
	}
	painter.setRenderHint(QPainter::Antialiasing);
	QColor fg(QApplication::palette().color(QPalette::Text));
	m_scatterplot->settings.tickLabelColor = fg;

	for (double x : m_xMarker.keys())
	{
		QColor color = m_xMarker[x].first;
		Qt::PenStyle penStyle = m_xMarker[x].second;
		QPen p(color, 1, penStyle);
		painter.setPen(p);
		QLine line;
		int pos = static_cast<int>(m_scatterplot->getRect().left() +  m_scatterplot->p2x(x));
		line.setP1(QPoint(pos, m_scatterplot->getRect().top()));
		line.setP2(QPoint(pos, m_scatterplot->getRect().top()+m_scatterplot->getRect().height()));
		painter.drawLine(line);
	}

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
	painter.drawText(QRectF(PaddingLeft(), height() - fm.height() - TextPadding, width()-PaddingLeft(), fm.height()),
			Qt::AlignHCenter | Qt::AlignTop, m_data->parameterName(m_scatterplot->getIndices()[0]));
	painter.rotate(-90);
	painter.drawText(QRectF(-(height()-PaddingBottom()), 0, height()-PaddingBottom(), fm.height()), Qt::AlignHCenter | Qt::AlignTop, m_data->parameterName(m_scatterplot->getIndices()[1]));
	painter.restore();

	if (m_showTooltip)
	{
		drawTooltip(painter);
	}
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
	QColor popupFillColor(QApplication::palette().color(QPalette::Window));
	painter.setBrush(popupFillColor);
	QColor popupBorderColor(QApplication::palette().color(QPalette::Dark));
	painter.setPen(popupBorderColor);
	painter.translate(popupPos);
	QString text = "<b>#" + QString::number(curInd) + "</b><br> " + m_pointInfo->text(pInds, curInd);
	double docScale = 1.0;
	QTextDocument doc;
	doc.setHtml(text);
	int popupWidth = 200;	// = settings.popupWidth
	doc.setTextWidth(popupWidth);
	double tipDim[2] = { 5, 10 }; // = settings.popupTipDim
	double popupWidthHalf = popupWidth * docScale / 2; // settings.popupWidth / 2
	auto popupHeight = doc.size().height() * docScale;
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
	QColor popupTextColor(QApplication::palette().color(QPalette::ToolTipText));  // = settings.popupTextColor;
	ctx.palette.setColor(QPalette::Text, popupTextColor);
	painter.scale(docScale, docScale);
	doc.documentLayout()->draw(&painter, ctx); //doc.drawContents( &painter );
	painter.restore();
}

void iAScatterPlotWidget::adjustScatterPlotSize()
{
	if (!m_scatterplot)
	{
		LOG(lvlError, "No scatterplot!");
		return;
	}
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
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
	{
		m_scatterplot->SPLOMWheelEvent(event);
		update();
	}
}

void iAScatterPlotWidget::mousePressEvent(QMouseEvent * event)
{
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
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
		auto oldHighlights(m_viewData->highlightedPoints());
		m_viewData->clearHighlightedPoints();
		for (auto idx : oldHighlights)
		{
			emit pointHighlighted(idx, false);
		}
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

void iAScatterPlotWidget::setHighlightColor(QColor color)
{
	m_scatterplot->setHighlightColor(color);
}

void iAScatterPlotWidget::setHighlightColorTheme(iAColorTheme const* theme)
{
	m_scatterplot->setHighlightColorTheme(theme);
}

void iAScatterPlotWidget::setHighlightDrawMode(iAScatterPlot::HighlightDrawModes drawMode)
{
	m_scatterplot->setHighlightDrawMode(drawMode);
}

void iAScatterPlotWidget::mouseReleaseEvent(QMouseEvent * event)
{
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
	{
		m_scatterplot->SPLOMMouseReleaseEvent(event);
		update();
	}
}

void iAScatterPlotWidget::mouseMoveEvent(QMouseEvent * event)
{
	QPointF p = event->position();
	if (p.x() >= PaddingLeft() && p.x() <= (width() - PaddingRight) &&
	    p.y() >= PaddingTop    && p.y() <= (height() - PaddingBottom()))
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

namespace
{
	void updateParamMenuCheckState(QMenu* menu, std::shared_ptr<iASPLOMData> data, size_t visibleIdx)
	{
		for (auto col : menu->actions())
		{
			size_t paramIdx = data->paramIndex(col->text());
			if (paramIdx >= data->numParams())
			{
				LOG(lvlWarn,
					QString("Invalid menu entry %1 in column pick submenu - there is currently no such column!")
						.arg(col->text()));
				continue;
			}
			QSignalBlocker toggleBlock(col);
			col->setChecked(paramIdx == visibleIdx);
		}
	}
}

void iAScatterPlotWidget::contextMenuEvent(QContextMenuEvent* event)
{
	if (m_contextMenu)
	{
		updateParamMenuCheckState(m_xMenu, m_data, m_scatterplot->getIndices()[0]);
		updateParamMenuCheckState(m_yMenu, m_data, m_scatterplot->getIndices()[1]);
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

void iAScatterPlotWidget::setShowToolTips(bool enabled)
{
	m_showTooltip = enabled;
}

void iAScatterPlotWidget::setPointInfo(std::shared_ptr<iAScatterPlotPointInfo> pointInfo)
{
	m_pointInfo = pointInfo;
}
