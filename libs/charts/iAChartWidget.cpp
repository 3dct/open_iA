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
#include "iAChartWidget.h"

#include "iAAttributeDescriptor.h"
#include "iAFileUtils.h"
#include "iALog.h"
#include "iAMapperImpl.h"
#include "iAMathUtility.h"
#include "iAPlot.h"
#include "iAPlotData.h"
#include "iAQGLWidget.h"
#include "iAStringHelper.h"

#include <vtkMath.h>

#include <QAction>
#include <QApplication>    // for qApp->palette()
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#if (defined(CHART_OPENGL) && defined(OPENGL_DEBUG))
#include <QOpenGLDebugLogger>
#endif
#ifdef CHART_OPENGL
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#endif
#include <QPainter>
#include <QRubberBand>
#include <QtGlobal> // for QT_VERSION
#include <QToolTip>
#include <QWheelEvent>
#include <QWindow>

#include <fstream>

namespace
{
	const double ZoomXMin = 1.0;
	const double ZoomXMax = 2048;
	const double ZoomXMaxEmpty = 1.0;
	const double ZoomYMin = 0.1;		// the y-axis is not adapted (yet) to work with zoom < 1.0
	const double ZoomYMax = 32768;
	const double ZoomXStep = 1.5;
	const double ZoomYStep = 1.5;
	const int CategoricalFontSize = 7;
	const int MarginLeft = 5;
	const int MarginBottom = 5;
	const int AxisTicksYMax = 5;
	const int AxisTicksXDefault = 2;
	const int TickWidth = 6;

	const double LogYMapModeMin = 0.5;

	int markerPos(int x, size_t step, size_t stepCount)
	{
		if (step == stepCount)
		{
			--x;
		}
		return x;
	}

	int textPos(int markerX, size_t step, size_t stepCount, int textWidth)
	{
		return (step == 0)
			? markerX                  // right aligned to indicator line
			: (step < stepCount)
			? markerX - textWidth / 2  // centered to the indicator line
			: markerX - textWidth;     // left aligned to the indicator line
	}

	void ensureNonZeroRange(double* bounds, bool warn = false, double offset = 0.1)
	{
		if (dblApproxEqual(bounds[0], bounds[1]))
		{
			if (warn)
			{
				LOG(lvlWarn, QString("range [%1..%2] invalid (min~=max), enlarging it by %3").arg(bounds[0]).arg(bounds[1]).arg(offset));
			}
			bounds[0] -= offset;
			bounds[1] += offset;
		}
	}
}

iAChartWidget::iAChartWidget(QWidget* parent, QString const & xLabel, QString const & yLabel):
	iAChartParentWidget(parent),
	m_xCaption(xLabel),
	m_yCaption(yLabel),
	m_xZoom(1.0),
	m_yZoom(1.0),
	m_xZoomStart(1.0),
	m_yZoomStart(1.0),
	m_xShift(0.0),
	m_xShiftStart(0.0),
	m_translationY(0),
	m_translationStartY( 0 ),
	m_mode(NO_MODE),
	m_yMappingMode(Linear),
	m_contextMenu(new QMenu(this)),
	m_showTooltip(true),
	m_showXAxisLabel(true),
	m_fontHeight(0),
	m_yMaxTickLabelWidth(0),
	m_customXBounds(false),
	m_customYBounds(false),
	m_captionPosition(Qt::AlignHCenter | Qt::AlignBottom),
	m_selectionMode(SelectionDisabled),
	m_selectionBand(new QRubberBand(QRubberBand::Rectangle, this)),
	m_maxXAxisSteps(AxisTicksXDefault),
	m_drawXAxisAtZero(false),
	m_emptyText("Chart not (yet) available.")
{
#ifdef CHART_OPENGL
	setFormat(defaultQOpenGLWidgetFormat());
#endif
	updateBounds();
	setMouseTracking(true);
	setFocusPolicy(Qt::WheelFocus);
	setAutoFillBackground(false);
	m_selectionBand->hide();
}

iAChartWidget::~iAChartWidget()
{
	delete m_contextMenu;
}

void iAChartWidget::wheelEvent(QWheelEvent *event)
{
	if (event->angleDelta().x() != 0)  // by taking .x() here, we allow to either
	{	// use a secondary mouse wheel axis (horizontal), or the Alt modifier key
		// (which in Qt makes primary mouse wheel act like secondary) to be used
		zoomAlongY(event->angleDelta().x(), true);
	}
	else
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
		zoomAlongX(event->angleDelta().y(), event->position().x(), true);
#else
		zoomAlongX(event->angleDelta().y(), event->x(), true);
#endif
	}
	event->accept();
	update();
}

void iAChartWidget::leaveEvent(QEvent*)
{
	this->clearFocus();
}

void iAChartWidget::zoomAlongY(double value, bool deltaMode)
{
	if (deltaMode)
	{
		if (value /* = delta */ > 0)
		{
			value = m_yZoom * ZoomYStep;
		}
		else
		{
			value = m_yZoom / ZoomYStep;
		}
	}
	double yZoomBefore = m_yZoom;
	m_yZoom = clamp(ZoomYMin, ZoomYMax, value);
	if (!dblApproxEqual(yZoomBefore, m_yZoom))
	{
		emit axisChanged();
	}
}

void iAChartWidget::zoomAlongX(double value, int x, bool deltaMode)
{
	// don't do anything if we're already at the limit
	if ((deltaMode && ((value <  0   && m_xZoom == 1.0) || (value > 0           && dblApproxEqual(m_xZoom, maxXZoom())))) ||
	   (!deltaMode && ((value <= 1.0 && m_xZoom == 1.0) || (value >= maxXZoom() && dblApproxEqual(m_xZoom, maxXZoom())))))
	{
		return;
	}
	double xZoomBefore = m_xZoom;
	double xShiftBefore = m_xShift;
	double fixedDataX = mouse2DataX(x - leftMargin());
	if (deltaMode)
	{
		if (value /* = delta */ > 0)
		{
			value = m_xZoom * ZoomXStep;
		}
		else
		{
			value = m_xZoom /= ZoomXStep;
		}
	}
	m_xZoom = clamp(ZoomXMin, maxXZoom(), value);
	m_xMapper->update(m_xBounds[0], m_xBounds[1], 0, fullChartWidth());
	m_xShift = limitXShift(fixedDataX - xMapper().dstToSrc(x - leftMargin()));
	if (!dblApproxEqual(xZoomBefore, m_xZoom) || !dblApproxEqual(m_xShift, xShiftBefore))
	{
		emit axisChanged();
	}
}

void iAChartWidget::setXZoom(double xZoom)
{
	m_xZoom = xZoom;
}

void iAChartWidget::setXShift(double xShift)
{
	m_xShift = xShift;
}

void iAChartWidget::setYZoom(double yZoom)
{
	m_yZoom = yZoom;
}

int iAChartWidget::chartWidth() const
{
	return geometry().width() - leftMargin();
}

//! width in pixels that the chart would have if it were fully shown (considering current zoom level)k
double iAChartWidget::fullChartWidth() const
{
	return chartWidth() * m_xZoom;
}

int iAChartWidget::chartHeight() const
{
	return geometry().height() - bottomMargin();
}

void iAChartWidget::setXCaption(QString const & caption)
{
	m_xCaption = caption;
}

void iAChartWidget::setYCaption(QString const & caption)
{
	m_yCaption = caption;
}

double const * iAChartWidget::xBounds() const
{
	return m_xBounds;
}

double iAChartWidget::xRange() const
{
	return m_xBounds[1] - m_xBounds[0];
}

int iAChartWidget::bottomMargin() const
{
	return MarginBottom + static_cast<int>((m_showXAxisLabel ? 2 : 1) * m_fontHeight);
}

int iAChartWidget::leftMargin() const
{
	return (m_yCaption == "") ? 0 :
		MarginLeft + m_fontHeight + m_yMaxTickLabelWidth + TickWidth;
}

iAPlotData::DataType iAChartWidget::minYDataValue(size_t startPlot) const
{
	iAPlotData::DataType minVal = std::numeric_limits<iAPlotData::DataType>::max();
	for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
	{
		minVal = std::min(m_plots[curPlot]->data()->yBounds()[0], minVal);
	}
	return minVal;
}

iAPlotData::DataType iAChartWidget::maxYDataValue(size_t startPlot) const
{
	iAPlotData::DataType maxVal = std::numeric_limits<iAPlotData::DataType>::lowest();
	for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
	{
		maxVal = std::max(m_plots[curPlot]->data()->yBounds()[1], maxVal);
	}
	return maxVal;
}

iAPlotData::DataType const * iAChartWidget::yBounds() const
{
	return m_yBounds;
}

iAMapper const & iAChartWidget::xMapper() const
{
	if (!m_xMapper || !m_yMapper)
	{
		createMappers();
	}
	return *m_xMapper.data();
}

iAMapper const & iAChartWidget::yMapper() const
{
	if (!m_xMapper || !m_yMapper)
	{
		createMappers();
	}
	return *m_yMapper.data();
}

int iAChartWidget::data2MouseX(double dataX)
{
	return xMapper().srcToDst(dataX - m_xShift);
}

double iAChartWidget::mouse2DataX(int mouseX)
{
	return xMapper().dstToSrc(mouseX) + m_xShift;
}

void iAChartWidget::createMappers() const
{
	m_xMapper = QSharedPointer<iALinearMapper>::create(m_xBounds[0], m_xBounds[1], 0, fullChartWidth());
	if (m_yMappingMode == Linear)
	{
		m_yMapper = QSharedPointer<iALinearMapper>::create(m_yBounds[0], m_yBounds[1], 0, (chartHeight() - 1) * m_yZoom);
	}
	else
	{
		m_yMapper = QSharedPointer<iALogarithmicMapper>::create(
			m_yBounds[0] > 0 ? m_yBounds[0] : LogYMapModeMin, m_yBounds[1], 0, (chartHeight() - 1) * m_yZoom);
		if (m_yBounds[0] < 0)
		{
			LOG(lvlWarn, QString("Invalid y bounds in chart for logarithmic mapping: minimum=%1 is < 0, using %2 instead.")
				.arg(m_yBounds[0]).arg(LogYMapModeMin));
		}
	}
}

void iAChartWidget::drawImageOverlays(QPainter& painter)
{
	QRect targetRect = geometry();
	QRect chartRect = QRect(-xMapper().srcToDst(m_xShift), bottomMargin(), chartWidth(), chartHeight());
	int yTranslate = static_cast<int>(-(m_yZoom - 1) * (targetRect.height()));
	targetRect.setHeight(targetRect.height() - targetRect.top() - 1);
	targetRect.setWidth(static_cast<int>((targetRect.width() - leftMargin()) * m_xZoom));
	targetRect.setTop(targetRect.top() + yTranslate);
	targetRect.setLeft(0);
	for (size_t i = 0; i < m_overlays.size(); ++i)
	{
		painter.drawImage(m_overlays[i].second ? // stretch to full chart area?
			targetRect : chartRect,
			*(m_overlays[i].first.data()), m_overlays[i].first->rect());
	}
}

void iAChartWidget::drawAfterPlots(QPainter& /*painter*/)
{}

// TODO: unify with dblToStringWithUnits...?
QString iAChartWidget::xAxisTickMarkLabel(double value, double stepWidth)
{
	int placesBeforeComma = requiredDigits(value);
	int placesAfterComma = (stepWidth < 10) ? requiredDigits(10 / stepWidth) : 0;
	if ((!m_plots.empty() && m_plots[0]->data()->valueType() == iAValueType::Continuous) || placesAfterComma > 1)
	{
		QString result = QString::number(value, 'g', ((value > 0) ? placesBeforeComma + placesAfterComma : placesAfterComma));
		if (result.contains("e")) // only 4 digits for scientific notation:
		{
			result = QString::number(value, 'g', 4);
		}
		return result;
	}
	else
	{ // not ideal;
		return QString::number(value, 'f', 0);
	}
}

void iAChartWidget::drawAxes(QPainter& painter)
{
	drawXAxis(painter);
	drawYAxis(painter);
}

bool iAChartWidget::categoricalAxis() const
{
	if (!m_plots.empty())
	{
		return (m_plots[0]->data()->valueType() == iAValueType::Categorical);
	}
	else
	{
		return false;
	}
}

double iAChartWidget::visibleXStart() const
{
	return xBounds()[0] + m_xShift;
}

double iAChartWidget::visibleXEnd() const
{
	double visibleRange = xRange() / m_xZoom;
	return visibleXStart() + visibleRange;
}

void iAChartWidget::drawXAxis(QPainter &painter)
{
	painter.setPen(qApp->palette().color(QPalette::Text));
	QFontMetrics fm = painter.fontMetrics();
	size_t stepCount = m_maxXAxisSteps;
	double stepWidth;
	double startXVal = clamp(m_xBounds[0], m_xBounds[1], visibleXStart());
	double endXVal = clamp(m_xBounds[0], m_xBounds[1], visibleXEnd());
	if (!categoricalAxis())
	{
		// check for overlap:
		bool overlap;
		do
		{
			stepWidth = xRange() / stepCount;
			overlap = false;
			for (size_t i = 0; i<stepCount && !overlap; ++i)
			{
				double value = m_xBounds[0] + static_cast<double>(i) * stepWidth;
				double nextValue = m_xBounds[0] + static_cast<double>(i+1) * stepWidth;
				if (value < startXVal)
				{
					continue;
				}
				else if (value > endXVal)
				{
					break;
				}
				QString text = xAxisTickMarkLabel(value, stepWidth);
				int markerX = markerPos(static_cast<int>(xMapper().srcToDst(value)), i, stepCount);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
				int textX = textPos(markerX, i, stepCount, fm.horizontalAdvance(text));
				int nextMarkerX = markerPos(static_cast<int>(xMapper().srcToDst(nextValue)), i + 1, stepCount);
				int nextTextX = textPos(nextMarkerX, i + 1, stepCount, fm.horizontalAdvance(text));
				int textWidth = fm.horizontalAdvance(text+"M");
#else
				int textX = textPos(markerX, i, stepCount, fm.width(text));
				int nextMarkerX = markerPos(xMapper().srcToDst(nextValue), i + 1, stepCount);
				int nextTextX = textPos(nextMarkerX, i + 1, stepCount, fm.width(text));
				int textWidth = fm.width(text + "M");
#endif
				overlap = (textX + textWidth) >= nextTextX;
			}
			if (overlap)
			{
				stepCount /= 2;
			}
		} while (overlap && stepCount > 1);
	}
	else
	{
		QFont font = painter.font();
		font.setPointSize(CategoricalFontSize);
		painter.setFont(font);
		fm = painter.fontMetrics();
		stepWidth = xRange() / stepCount;
	}
	stepCount = std::max(static_cast<size_t>(1), stepCount); // at least one step
	for (size_t i = 0; i <= stepCount; ++i)
	{
		double value = m_xBounds[0] + static_cast<double>(i) * stepWidth;
		if (value < startXVal)
		{
			continue;
		}
		else if (value > endXVal)
		{
			break;
		}
		QString text = xAxisTickMarkLabel(value, stepWidth);
		int markerX = markerPos(static_cast<int>(xMapper().srcToDst(value)), i, stepCount);
		painter.drawLine(markerX, TickWidth, markerX, -1);
		int textWidth = 1 +
			// + 1 is required - apparently width of QRect passed to drawRect below
			// needs to be larger than that width the text actually requires.
			// Without it, we often get text output like "0." where e.g. "0.623" would be the actual text
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
			fm.horizontalAdvance(text);
#else
			fm.width(text);
#endif
		int textX = textPos(markerX, i, stepCount, textWidth);
		int textY = TickWidth;
		painter.drawText(QRect(textX, textY, textWidth, m_fontHeight), text);
	}

	//draw the x axis
	painter.setPen(qApp->palette().color(QPalette::Text));
	int xAxisStart = xMapper().srcToDst(visibleXStart());
	painter.drawLine(xAxisStart, -1, xAxisStart+chartWidth(), -1);
	if (m_drawXAxisAtZero && std::abs(-1.0-yMapper().srcToDst(0)) > 5) // if axis at bottom is at least 5 pixels away from zero point, draw additional line
	{
		painter.drawLine(xAxisStart, static_cast<int>(-yMapper().srcToDst(0)), xAxisStart+chartWidth(), static_cast<int>(-yMapper().srcToDst(0)));
	}
	if (m_showXAxisLabel)
	{
		QRect textRect(0,
			m_captionPosition.testFlag(Qt::AlignBottom)	?
				bottomMargin() - m_fontHeight - 1 : // Bottom
				-chartHeight(),                     // Top
			chartWidth(), m_fontHeight);
		painter.drawText(textRect, m_captionPosition.testFlag(Qt::AlignHCenter)? Qt::AlignHCenter: Qt::AlignLeft, m_xCaption);
	}
}

void iAChartWidget::drawYAxis(QPainter &painter)
{
	if (leftMargin() <= 0)
	{
		return;
	}
	painter.save();
	painter.translate(xMapper().srcToDst(visibleXStart()), 0);
	QColor bgColor = qApp->palette().color(QWidget::backgroundRole());
	painter.fillRect(QRect(-leftMargin(), -chartHeight(), leftMargin(), geometry().height()), bgColor);
	QFontMetrics fm = painter.fontMetrics();
	int aheight = chartHeight() - 1;
	painter.setPen(qApp->palette().color(QPalette::Text));

	// at most, make Y_AXIS_STEPS, but reduce to number actually fitting in current height:
	int stepNumber = std::min(AxisTicksYMax, static_cast<int>(aheight / (m_fontHeight*1.1)));
	stepNumber = std::max(1, stepNumber);	// make sure there's at least 2 steps
	const double step = 1.0 / (stepNumber * m_yZoom);

	for (int i = 0; i <= stepNumber; ++i)
	{
		double pos = step * i;
		int y = -static_cast<int>(pos * aheight * m_yZoom) - 1;
		double yValue = yMapper().dstToSrc(-y-1);
		QString text = dblToStringWithUnits(yValue, 10);
		painter.drawLine(static_cast<int>(-TickWidth), y, 0, y);	// indicator line
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		painter.drawText( - ( fm.horizontalAdvance(text) + TickWidth),
#else
		painter.drawText( - ( fm.width(text) + TickWidth),
#endif
			(i == stepNumber) ? y + static_cast<int>(0.75*m_fontHeight) // write the text top aligned to the indicator line
			: y + static_cast<int>(0.25*m_fontHeight)                   // write the text centered to the indicator line
			, text);
	}
	painter.drawLine(0, -1, 0, -aheight);
	//write the y axis label
	painter.save();
	painter.rotate(-90);
	QPointF textPos(
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		aheight*0.5 - 0.5*fm.horizontalAdvance(m_yCaption),
#else
		aheight*0.5 - 0.5*fm.width(m_yCaption),
#endif
		-leftMargin() + m_fontHeight - 5);
	painter.drawText(textPos, m_yCaption);
	painter.restore();
	painter.restore();
}

void iAChartWidget::setXBounds(double valMin, double valMax)
{
	m_customXBounds = true;
	m_xBounds[0] = valMin;
	m_xBounds[1] = valMax;
	//m_xTickBounds[0] = valMin;
	//m_xTickBounds[1] = valMax;
	ensureNonZeroRange(m_xBounds, true);
}

void iAChartWidget::setYBounds(iAPlotData::DataType valMin, iAPlotData::DataType valMax)
{
	m_customYBounds = true;
	m_yBounds[0] = valMin;
	m_yBounds[1] = valMax;
	ensureNonZeroRange(m_yBounds, true);
}

void iAChartWidget::resetYBounds()
{
	m_customYBounds = false;
	updateYBounds();
}

void iAChartWidget::updateYBounds(size_t startPlot)
{
	if (m_customYBounds)
	{
		return;
	}
	m_yBounds[0] = (m_plots.empty()) ? 0 :
		((startPlot != 0)
			? std::min(m_yBounds[0], minYDataValue(startPlot)) // partial update
			: minYDataValue(startPlot) );                      // full update
	m_yBounds[1] = (m_plots.empty()) ? 1 :
		((startPlot != 0)
			? std::max(m_yBounds[1], maxYDataValue(startPlot)) // partial update
			: maxYDataValue(startPlot) );                      // full update
	ensureNonZeroRange(m_yBounds);
}

void iAChartWidget::updateXBounds(size_t startPlot)
{
	if (m_customXBounds)
	{
		return;
	}
	if (m_plots.empty())
	{
		m_xBounds[0] = 0;
		m_xBounds[1] = 1;
		m_maxXAxisSteps = AxisTicksXDefault;
	}
	else
	{                             // update   partial            full
		m_xBounds[0]     = (startPlot != 0) ? m_xBounds[0]     : std::numeric_limits<double>::max();
		m_xBounds[1]     = (startPlot != 0) ? m_xBounds[1]     : std::numeric_limits<double>::lowest();
		m_maxXAxisSteps = 0;
		for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
		{
			auto d = m_plots[curPlot]->data();
			m_xBounds[0] = std::min(m_xBounds[0], d->xBounds()[0] /*- ((d->valueType() == iAValueType::Discrete) ? 0.5 : 0)*/ );
			m_xBounds[1] = std::max(m_xBounds[1], d->xBounds()[1] /*+ ((d->valueType() == iAValueType::Discrete) ? 0.5 : 0)*/ );
			m_maxXAxisSteps = std::max(m_maxXAxisSteps, d->valueCount() );
		}
		ensureNonZeroRange(m_xBounds);
	}
}

void iAChartWidget::updateBounds(size_t startPlot)
{
	updateXBounds(startPlot);
	updateYBounds(startPlot);
}

void iAChartWidget::resetView()
{
	m_xShift = 0.0;
	m_xZoom = 1.0;
	m_yZoom = 1.0;
	emit axisChanged();
	update();
}


// @{ TODO: these two methods need to be made plot-specific!
long iAChartWidget::screenX2DataBin(int x) const
{
	if (m_plots.empty())
	{
		return x;
	}
	double numBin = m_plots[0]->data()->valueCount();
	double dBinX = clamp(0.0, numBin-1, static_cast<double>(x + xMapper().srcToDst(m_xShift) - leftMargin()) * numBin / fullChartWidth());
	long binX = static_cast<long>(dBinX);
	return binX;
}

int iAChartWidget::dataBin2ScreenX(long x) const
{
	if (m_plots.empty())
	{
		assert(x > std::numeric_limits<int>::lowest() && x < std::numeric_limits<int>::max());
		return static_cast<int>(x);
	}
	double numBin = m_plots[0]->data()->valueCount();
	double screenX = static_cast<double>(x) * fullChartWidth() / (numBin);
	screenX = clamp(0.0, fullChartWidth(), screenX);
	return static_cast<int>(screenX);
}

bool iAChartWidget::isTooltipShown() const
{
	return m_showTooltip;
}

QPoint iAChartWidget::contextMenuPos() const
{
	return m_contextPos;
}

double iAChartWidget::maxXZoom() const
{
	if (m_plots.empty())
	{
		return ZoomXMaxEmpty;
	}
	double valueCount = m_plots[0]->data()->valueCount();
	return std::max(std::min(ZoomXMax, valueCount), 1.0);
}

void iAChartWidget::setYMappingMode(AxisMappingType drawMode)
{
	if (m_yMappingMode == drawMode)
	{
		return;
	}
	m_yMappingMode = drawMode;
	createMappers();
}

void iAChartWidget::setCaptionPosition(Qt::Alignment captionAlignment)
{
	m_captionPosition = captionAlignment;
}

void iAChartWidget::setShowXAxisLabel(bool show)
{
	m_showXAxisLabel = show;
}

void iAChartWidget::addPlot(QSharedPointer<iAPlot> plot)
{
	assert(plot);
	if (!plot)
	{
		LOG(lvlInfo, "Trying to add empty plot!");
		return;
	}
	if (std::find(m_plots.begin(), m_plots.end(), plot) != m_plots.end())
	{
		LOG(lvlInfo, "Trying to add plot a second time!");
		return;
	}
	m_plots.push_back(plot);
	updateBounds(m_plots.size()-1);
}

void iAChartWidget::removePlot(QSharedPointer<iAPlot> plot)
{
	if (!plot)
	{
		return;
	}
	auto it = std::find(m_plots.begin(), m_plots.end(), plot);
	if (it != m_plots.end())
	{
		m_plots.erase(it);
		updateBounds();
	}
}

void iAChartWidget::clearPlots()
{
	m_plots.clear();
}

std::vector<QSharedPointer<iAPlot> > const & iAChartWidget::plots()
{
	return m_plots;
}

void iAChartWidget::addImageOverlay(QSharedPointer<QImage> imgOverlay, bool stretch)
{
	m_overlays.push_back(std::make_pair(imgOverlay, stretch));
}

void iAChartWidget::removeImageOverlay(QImage * imgOverlay)
{
	for (auto it = m_overlays.begin(); it != m_overlays.end(); ++it)
	{
		if (it->first.data() == imgOverlay)
		{
			m_overlays.erase(it);
			break;
		}
	}
}

void iAChartWidget::clearImageOverlays()
{
	m_overlays.clear();
}

void iAChartWidget::setSelectionMode(SelectionMode mode)
{
	m_selectionMode = mode;
}

void iAChartWidget::drawPlots(QPainter &painter)
{
	if (m_plots.empty())
	{
		painter.scale(1, -1);
		painter.drawText(QRect(0, 0, chartWidth(), -chartHeight()), Qt::AlignCenter, "Chart not (yet) available.");
		painter.scale(1, -1);
		return;
	}
	double xStart = visibleXStart(), xEnd = visibleXEnd();
	for (auto plot: m_plots)
	{
		if (plot->visible())
		{
			size_t startIdx = clamp(static_cast<size_t>(0), plot->data()->valueCount()-1, plot->data()->nearestIdx(xStart));
			size_t endIdx = clamp(static_cast<size_t>(0), plot->data()->valueCount()-1, plot->data()->nearestIdx(xEnd)+1);
			plot->draw(painter, startIdx, endIdx, xMapper(), yMapper());
		}
	}
}

bool iAChartWidget::event(QEvent *event)
{
	if (event->type() != QEvent::ToolTip)
	{
		return iAChartParentWidget::event(event);
	}
	// maybe use mouseMove / setToolTip instead?
	if (m_plots.empty() || !m_showTooltip)
	{
		QToolTip::hideText();
		event->ignore();
	}
	QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
	showDataTooltip(helpEvent);
	return true;
}

void iAChartWidget::showDataTooltip(QHelpEvent* event)
{
	if (m_plots.empty() || !m_xMapper)
	{
		return;
	}
	int mouseX = clamp(0, geometry().width(), event->x()) - leftMargin();
	double dataX = mouse2DataX(mouseX);
	QString toolTipText;
	for (auto const & plot : m_plots)
	{
		if (plot->visible() && inRange(plot->data()->xBounds(), dataX) )
		{
			toolTipText += plot->data()->toolTipText(dataX) + "\n";
		}
	}
	QToolTip::showText(event->globalPos(), toolTipText.trimmed(), this);
}

void iAChartWidget::changeMode(int newMode, QMouseEvent *event)
{
	m_mode = newMode;
	if (newMode == MOVE_VIEW_MODE)
	{
		m_xShiftStart = m_xShift;
		m_translationStartY = m_translationY;
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
		m_dragStartPosX = event->x();
		m_dragStartPosY = event->y();
#else
		m_dragStartPosX = event->position().x();
		m_dragStartPosY = event->position().y();
#endif
	}
}

void iAChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (m_selectionBand->isVisible())
		{
			m_selectionBand->hide();
			QRectF dataRect;
			QRectF selectionRect(m_selectionBand->geometry());     // height-y because we are drawing reversed from actual y direction
			dataRect.setTop(   yMapper().dstToSrc(chartHeight() - selectionRect.bottom()) );
			dataRect.setBottom(yMapper().dstToSrc(chartHeight() - selectionRect.top()   ) );
			dataRect.setLeft(  mouse2DataX(static_cast<int>(selectionRect.left()-leftMargin())  ) );
			dataRect.setRight( mouse2DataX(static_cast<int>(selectionRect.right()-leftMargin()) ) );
			dataRect = dataRect.normalized();
			if (dataRect.top() < yBounds()[0])
			{
				dataRect.setTop(yBounds()[0]);
			}
			if (dataRect.bottom() > yBounds()[1])
			{
				dataRect.setBottom(yBounds()[1]);
			}
			m_selectedPlots.clear();
			double yMin = dataRect.top(), yMax = dataRect.bottom();
			// move to iAPlotData?
			for (size_t plotIdx=0; plotIdx<m_plots.size(); ++plotIdx)
			{
				if (!m_plots[plotIdx]->visible())
				{
					continue;
				}
				size_t startIdx = std::max(static_cast<size_t>(0), m_plots[plotIdx]->data()->nearestIdx(dataRect.left()));
				size_t endIdx = std::min(m_plots[plotIdx]->data()->valueCount(), m_plots[plotIdx]->data()->nearestIdx(dataRect.right()+1));
				for (size_t idx = startIdx; idx < endIdx; ++idx)
				{
					double yValue = m_plots[plotIdx]->data()->yValue(idx);
					if (yMin < yValue && yValue < yMax)
					{
						m_selectedPlots.push_back(plotIdx);
						break;
					}
				}
			}
			emit plotsSelected(m_selectedPlots);
		}
		update();
	}
	this->m_mode = NO_MODE;
}

void iAChartWidget::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
	emit dblClicked();
}

void iAChartWidget::mousePressEvent(QMouseEvent *event)
{
	switch(event->button())
	{
	case Qt::LeftButton:
		if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
			((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) &&
			((event->modifiers() & Qt::AltModifier) == Qt::AltModifier))
		{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
			m_zoomYPos = event->y();
#else
			m_zoomYPos = event->position().y();
#endif
			m_yZoomStart = m_yZoom;
			changeMode(Y_ZOOM_MODE, event);
		}
		else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
			((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
		{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
			m_zoomXPos = event->x();
			m_zoomYPos = event->y();
#else
			m_zoomXPos = event->position().x();
			m_zoomYPos = event->position().y();
#endif
			m_xZoomStart = m_xZoom;
			changeMode(X_ZOOM_MODE, event);
		}
		else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
		{
			changeMode(MOVE_VIEW_MODE, event);
		}
		else if (m_selectionMode == SelectPlot)
		{
			m_selectionOrigin = event->pos();
			m_selectionBand->setGeometry(QRect(m_selectionOrigin, QSize()));
			m_selectionBand->show();
		}
		break;
	case Qt::RightButton:
		update();
		break;
	case Qt::MiddleButton:
		changeMode(MOVE_VIEW_MODE, event);
		break;
	default:
		break;
	}

#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	emit clicked(mouse2DataX(event->x()-leftMargin()), /*yMapper().dstToSrc(event->y()), */event->modifiers());
#else
	emit clicked(mouse2DataX(event->position().x() - leftMargin()), /*yMapper().dstToSrc(event->y()), */ event->modifiers());
#endif
}

double iAChartWidget::limitXShift(double newXShift)
{
	return clamp(0.0, (xZoom() - 1) / (xZoom()) * xRange(), newXShift);
}

void iAChartWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch(m_mode)
	{
	case NO_MODE:
		if (m_selectionBand->isVisible())
		{
			m_selectionBand->setGeometry(QRect(m_selectionOrigin, event->pos()).normalized());
		}
		/* do nothing */ break;
	case MOVE_VIEW_MODE:
	{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
		int xDelta = m_dragStartPosX - event->x();
#else
		int xDelta = m_dragStartPosX - event->position().x();
#endif
		double dataDelta = xMapper().dstToSrc(xDelta) - m_xBounds[0];
		m_xShift = limitXShift(m_xShiftStart + dataDelta);
		emit axisChanged();
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
		m_translationY = m_translationStartY + event->y() - m_dragStartPosY;
#else
		m_translationY = m_translationStartY + event->position().y() - m_dragStartPosY;
#endif
		m_translationY = clamp(static_cast<int>(-(geometry().height() * m_yZoom - geometry().height())),
				static_cast<int>(geometry().height() * m_yZoom - geometry().height()), m_translationY);
		update();
	}
		break;
	case X_ZOOM_MODE:
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
		zoomAlongX(((m_zoomYPos-event->y())/2.0)+ m_xZoomStart, m_zoomXPos, false);
#else
		zoomAlongX(((m_zoomYPos - event->position().y()) / 2.0) + m_xZoomStart, m_zoomXPos, false);
#endif
		update();
		break;
	case Y_ZOOM_MODE:
		{
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
			int diff = static_cast<int>((m_zoomYPos-event->y())/2.0);
#else
			int diff = static_cast<int>((m_zoomYPos - event->position().y()) / 2.0);
#endif
			if (diff < 0)
			{
				zoomAlongY(-pow(ZoomYStep,-diff)+ m_yZoomStart, false);
			}
			else
			{
				zoomAlongY(pow(ZoomYStep,diff)+ m_yZoomStart, false);
			}
			update();
		}
		break;
	}
}

QImage iAChartWidget::drawOffscreen()
{
#ifdef CHART_OPENGL
	QSurfaceFormat format;
	format.setMajorVersion(3);
	format.setMinorVersion(3);
	QWindow window;
	window.setSurfaceType(QWindow::OpenGLSurface);
	window.setFormat(format);
	window.create();
	QOpenGLContext context;
	context.setFormat(format);
	if (!context.create())
	{
		qFatal("Cannot create the requested OpenGL context!");
	}
	context.makeCurrent(&window);
	const QSize drawRectSize(width(), height());
	QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setSamples(4);
	fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	QOpenGLFramebufferObject fbo(drawRectSize, fboFormat);
	fbo.bind();
	QOpenGLPaintDevice device(drawRectSize);
#else
	QImage image(width(), height(), QImage::Format_RGB32);
#endif
	QPainter p;
#ifdef CHART_OPENGL
	p.begin(&device);
#else
	p.begin(&image);
#endif

	drawAll(p);
	p.end();
#ifdef CHART_OPENGL
	fbo.release();
	QImage image = fbo.toImage();
	context.doneCurrent();
#endif
	return image;
}

void iAChartWidget::setEmptyText(QString const& text)
{
	m_emptyText = text;
}

#ifdef CHART_OPENGL
void iAChartWidget::initializeGL()
{
	initializeOpenGLFunctions();
}

void iAChartWidget::paintGL()
#else
void iAChartWidget::paintEvent(QPaintEvent* /*event*/)
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
	QPainter p(this);
	QColor bgColor(qApp->palette().color(QWidget::backgroundRole()));
#ifdef CHART_OPENGL
	p.beginNativePainting();
	glClearColor(bgColor.red() / 255.0, bgColor.green() / 255.0, bgColor.blue() / 255.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	p.endNativePainting();
#else
	p.fillRect(rect(), bgColor);
#endif
	drawAll(p);
}

void iAChartWidget::drawAll(QPainter & painter)
{
	painter.setRenderHint(QPainter::Antialiasing);
	if (m_plots.empty())
	{
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, m_emptyText);
		return;
	}
	if (chartWidth() <= 1 || chartHeight() <= 1)
	{
		return;
	}
	QFontMetrics fm = painter.fontMetrics();
	m_fontHeight = fm.height();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	m_yMaxTickLabelWidth = fm.horizontalAdvance("4.44M");
#else
	m_yMaxTickLabelWidth = fm.width("4.44M");
#endif
	if (!m_xMapper || !m_yMapper)
	{
		createMappers();
	}
	m_xMapper->update(m_xBounds[0], m_xBounds[1], 0, fullChartWidth());
	m_yMapper->update(m_yMappingMode == Logarithmic && m_yBounds[0] <= 0 ? LogYMapModeMin : m_yBounds[0], m_yBounds[1],
		0, m_yZoom * (chartHeight() - 1));
	painter.save();
	painter.translate(-xMapper().srcToDst(visibleXStart()) + leftMargin(), -bottomMargin());
	drawImageOverlays(painter);
	//change the origin of the window to left bottom
	painter.translate(0, geometry().height());
	painter.scale(1, -1);

	drawPlots(painter);
	for (double x : m_xMarker.keys())
	{
		QColor color = m_xMarker[x].first;
		Qt::PenStyle penStyle = m_xMarker[x].second;
		QPen p(color, 1, penStyle);
		painter.setPen(p);
		QLine line;
		QRect diagram = geometry();
		int pos = static_cast<int>(xMapper().srcToDst(x));
		line.setP1(QPoint(pos, 0));
		line.setP2(QPoint(pos, diagram.height() - bottomMargin()));
		painter.drawLine(line);
	}

	drawAfterPlots(painter);

	painter.scale(1, -1);
	painter.setRenderHint(QPainter::Antialiasing, false);
	drawAxes(painter);
}

void iAChartWidget::setXMarker(double xPos, QColor const& color, Qt::PenStyle penStyle)
{
	m_xMarker[xPos] = qMakePair(color, penStyle);
}

void iAChartWidget::removeXMarker(double xPos)
{
	m_xMarker.remove(xPos);
}

void iAChartWidget::clearMarkers()
{
	m_xMarker.clear();
}

void iAChartWidget::addContextMenuEntries(QMenu* /*contextMenu*/)
{}

void iAChartWidget::contextMenuEvent(QContextMenuEvent *event)
{
	m_contextPos = event->pos();
	m_contextMenu->clear();
	m_contextMenu->addAction(QIcon(":/images/resetView.png"), tr("Reset histogram view"), this, &iAChartWidget::resetView);
	QAction *showTooltipAction = new QAction(tr("Show histogram coordinates"), this);
	showTooltipAction->setCheckable(true);
	showTooltipAction->setChecked(m_showTooltip);
	connect(showTooltipAction, &QAction::toggled, this, &iAChartWidget::showTooltip);
	m_contextMenu->addAction(showTooltipAction);
	m_contextMenu->addAction(QIcon(":/images/save.png"), tr("Export histogram data"), this, &iAChartWidget::exportData);
	m_contextMenu->addSeparator();
	addContextMenuEntries(m_contextMenu);
	m_contextMenu->exec(event->globalPos());
}

void iAChartWidget::exportData()
{
	if (m_plots.empty())
	{
		return;
	}
	int plotIdx = 0;
	if (m_plots.size() > 1)
	{
		LOG(lvlInfo, "More than one plot available, exporting only first!");
		/*
		iAParameterDlg::ParamListT params;
		addParameter(params, "Plot index", iAValueType::Discrete, 0, 0, m_plots.size());
		iAParameterDlg dlg(this, "Choose plot", params, "More than one plot available - please choose which one you want to export!");
		if (dlg.exec() != QDialog::Accepted)
		{
			return;
		}
		plotIdx = dlg.parameterValues()["Plot index"].toInt();
		*/
	}
	QString fileName = QFileDialog::getSaveFileName(
		this, tr("Save File"), QDir::currentPath(), tr("CSV (*.csv)"));
	if (fileName.isEmpty())
	{
		return;
	}
	std::ofstream out( getLocalEncodingFileName(fileName));
	out << m_xCaption.toStdString() << "," << QString("%1%2").arg(m_yCaption).arg(plotIdx).toStdString() << "\n";
	for (size_t idx = 0; idx < m_plots[plotIdx]->data()->valueCount(); ++idx)
	{
		out << QString::number(m_plots[plotIdx]->data()->xValue(idx), 'g', 15).toStdString()
			<< "," << QString::number(m_plots[plotIdx]->data()->yValue(idx), 'g', 15).toStdString()
			<< "\n";
	}
	out.close();
}

void iAChartWidget::showTooltip(bool toggled)
{
	m_showTooltip = toggled;
}

void iAChartWidget::setDrawXAxisAtZero(bool enable)
{
	m_drawXAxisAtZero = enable;
}
