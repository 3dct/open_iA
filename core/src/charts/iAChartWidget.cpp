/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAChartWidget.h"

#include "iAConsole.h"
#include "iAMapperImpl.h"
#include "iAMathUtility.h"
#include "iAPlot.h"
#include "iAPlotData.h"
#include "iAStringHelper.h"
#include "io/iAFileUtils.h"

#include <vtkMath.h>

#include <QAction>
#include <QDateTime>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
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
	const double ZoomYMin = 1.0;		// the y-axis is not adapted (yet) to work with zoom < 1.0
	const double ZoomYMax = 32768;
	const double ZoomXStep = 1.5;
	const double ZoomYStep = 1.5;
	const int CategoricalFontSize = 7;
	const int MarginLeft = 5;
	const int MarginBottom = 5;
	const int AxisTicksYMax = 5;
	const int AxisTicksXDefault = 2;
	const int TickWidth = 6;

	int requiredDigits(double value)
	{
		return (value >= -1.0 && value < 1.0) ?
			1 : std::floor(std::log10(std::abs(value))) + 1;
	}

	int markerPos(int x, size_t step, size_t stepCount)
	{
		if (step == stepCount) --x;
		return x;
	}

	int textPos(int markerX, size_t step, size_t stepCount, int textWidth)
	{
		return (step == 0)
			? markerX					// right aligned to indicator line
			: (step < stepCount)
			? markerX - textWidth / 2	// centered to the indicator line
			: markerX - textWidth;	// left aligned to the indicator line
	}

	void ensureNonZeroRange(double* bounds, bool warn = false, double offset = 0.1)
	{
		if (dblApproxEqual(bounds[0], bounds[1]))
		{
			if (warn)
				DEBUG_LOG(QString("range [%1..%2] invalid (min~=max), enlarging it by %3").arg(bounds[0]).arg(bounds[1]).arg(offset));
			bounds[0] -= offset;
			bounds[1] += offset;
		}
	}
}

iAChartWidget::iAChartWidget(QWidget* parent, QString const & xLabel, QString const & yLabel):
	iAQGLWidget(parent),
	m_xCaption(xLabel),
	m_yCaption(yLabel),
	m_xZoom(1.0),
	m_yZoom(1.0),
	m_xZoomStart(1.0),
	m_yZoomStart(1.0),
	m_translationX(0),
	m_translationY(0),
	m_translationStartX(0),
	m_translationStartY( 0 ),
	m_mode(NO_MODE),
	m_yMappingMode(Linear),
	m_contextMenuVisible(false),
	m_contextMenu(new QMenu(this)),
	m_showTooltip(true),
	m_showXAxisLabel(true),
	m_fontHeight(0),
	m_yMaxTickLabelWidth(0),
	m_customXBounds(false),
	m_customYBounds(false),
	m_captionPosition(Qt::AlignCenter | Qt::AlignBottom),
	m_selectionMode(SelectionDisabled),
	m_selectionBand(new QRubberBand(QRubberBand::Rectangle, this)),
	m_maxXAxisSteps(AxisTicksXDefault),
	m_drawXAxisAtZero(false)
{
	iAQGLFormat fmt;
	fmt.setSamples(8);
	setFormat(fmt);
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
	if ((event->modifiers() & Qt::AltModifier) == Qt::AltModifier ||
		(event->modifiers() & Qt::AltModifier) == Qt::Key_AltGr)
	{
		zoomAlongY(event->delta(), true);
	}
	else
	{
		zoomAlongX(event->delta(), event->x(), true);
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
			m_yZoom *= ZoomYStep;
		}
		else
		{
			m_yZoom /= ZoomYStep;
		}
	}
	else
	{
		m_yZoom = value;
	}
	m_yZoom = clamp(ZoomYMin, ZoomYMax, m_yZoom);
}

void iAChartWidget::zoomAlongX(double value, int x, bool deltaMode)
{
	int xZoomBefore = m_xZoom;
	int translationXBefore = m_translationX;
	// don't do anything if we're already at the limit
	if ( (deltaMode &&  ((value < 0    && m_xZoom == 1.0) || (value > 0           && m_xZoom == maxXZoom()))) ||
		 (!deltaMode && ((value <= 1.0 && m_xZoom == 1.0) || (value >= maxXZoom() && m_xZoom == maxXZoom()))) )
	{
		return;
	}
	int absoluteX = x-m_translationX-leftMargin();
	double absoluteXRatio = (double)absoluteX/((activeWidth()-1)*m_xZoom);
	if (deltaMode)
		if (value /* = delta */ > 0)
			m_xZoom *= ZoomXStep;
		else
			m_xZoom /= ZoomXStep;
	else
		m_xZoom = value;

	m_xZoom = clamp(ZoomXMin, maxXZoom(), m_xZoom);

	int absXAfterZoom = (int)(activeWidth()*m_xZoom*absoluteXRatio);

	m_translationX = clamp(-static_cast<int>(activeWidth() * (m_xZoom-1)), 0,
		-absXAfterZoom +x -leftMargin());

	if (xZoomBefore != m_xZoom || translationXBefore != m_translationX)
		emit xAxisChanged();
}

int iAChartWidget::activeWidth() const
{
	return geometry().width() - leftMargin();
}

int iAChartWidget::activeHeight() const
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
		minVal = std::min(m_plots[curPlot]->data()->yBounds()[0], minVal);
	return minVal;
}

iAPlotData::DataType iAChartWidget::maxYDataValue(size_t startPlot) const
{
	iAPlotData::DataType maxVal = std::numeric_limits<iAPlotData::DataType>::lowest();
	for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
		maxVal = std::max(m_plots[curPlot]->data()->yBounds()[1], maxVal);
	return maxVal;
}

iAPlotData::DataType const * iAChartWidget::yBounds() const
{
	return m_yBounds;
}

iAMapper const & iAChartWidget::xMapper() const
{
	return *m_xMapper.data();
}

iAMapper const & iAChartWidget::yMapper() const
{
	return *m_yMapper.data();
}

void iAChartWidget::createMappers()
{
	m_xMapper = QSharedPointer<iAMapper>(new iALinearMapper(m_xBounds[0], m_xBounds[1], 0, (activeWidth() - 1)*m_xZoom));
	if (m_yMappingMode == Linear)
		m_yMapper = QSharedPointer<iAMapper>(new iALinearMapper(m_yBounds[0], m_yBounds[1], 0, (activeHeight()-1)*m_yZoom));
	else
	{
		m_yMapper = QSharedPointer<iAMapper>(new iALogarithmicMapper(m_yBounds[0] > 0 ? m_yBounds[0] : 1, m_yBounds[1], 0, (activeHeight() - 1)*m_yZoom));
		if (m_yBounds[0] <= 0)
		{
			DEBUG_LOG(QString("Invalid y bounds in chart for logarithmic mapping: minimum=%1 is <= 0, using 1 instead.").arg(m_yBounds[0]));
		}
	}
}

void iAChartWidget::drawImageOverlays(QPainter& painter)
{
	QRect targetRect = geometry();
	int yTranslate = -(m_yZoom - 1) * (targetRect.height());
	targetRect.setHeight(targetRect.height() - targetRect.top() - 1);
	targetRect.setWidth((targetRect.width() - leftMargin()) * m_xZoom);
	targetRect.setTop(targetRect.top() + yTranslate);
	targetRect.setLeft(0);
	for (int i = 0; i < m_overlays.size(); ++i)
	{
		painter.drawImage(targetRect, *(m_overlays[i].data()), m_overlays[i]->rect());
	}
}

void iAChartWidget::drawAfterPlots(QPainter& /*painter*/)
{}

QString iAChartWidget::xAxisTickMarkLabel(double value, double stepWidth)
{
	int placesBeforeComma = requiredDigits(value);
	int placesAfterComma = (stepWidth < 10) ? requiredDigits(10 / stepWidth) : 0;
	if ((!m_plots.empty() && m_plots[0]->data()->valueType() == Continuous) || placesAfterComma > 1)
	{
		QString result = QString::number(value, 'g', ((value > 0) ? placesBeforeComma + placesAfterComma : placesAfterComma));
		if (result.contains("e")) // only 4 digits for scientific notation:
			result = QString::number(value, 'g', 4);
		return result;
	}
	else
		return QString::number(static_cast<long long>(value), 'g', 15);
}

void iAChartWidget::drawAxes(QPainter& painter)
{
	drawXAxis(painter);
	drawYAxis(painter);
}

bool iAChartWidget::categoricalAxis() const
{
	if (!m_plots.empty())
		return (m_plots[0]->data()->valueType() == Categorical);
	else
		return false;
}

double iAChartWidget::visibleXStart() const
{
	return xBounds()[0] + (((static_cast<double>(-m_translationX)) / (activeWidth()*m_xZoom)) * xRange());
}

double iAChartWidget::visibleXEnd() const
{
	double visibleRange = xRange() / m_xZoom;
	return visibleXStart() + visibleRange;
}

void iAChartWidget::drawXAxis(QPainter &painter)
{
	painter.setPen(QWidget::palette().color(QPalette::Text));
	const int TextAxisDistance = 2;
	QFontMetrics fm = painter.fontMetrics();
	size_t stepCount = m_maxXAxisSteps;
	double stepWidth;
	double startXVal = clamp(m_xTickBounds[0], m_xTickBounds[1], visibleXStart());
	double endXVal = clamp(m_xTickBounds[0], m_xTickBounds[1], visibleXEnd());
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
				double value = m_xTickBounds[0] + static_cast<double>(i) * stepWidth;
				double nextValue = m_xTickBounds[0] + static_cast<double>(i+1) * stepWidth;
				if (value < startXVal)
					continue;
				else if (value > endXVal)
					break;
				QString text = xAxisTickMarkLabel(value, stepWidth);
				int markerX = markerPos(m_xMapper->srcToDst(value), i, stepCount);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
				int textX = textPos(markerX, i, stepCount, fm.horizontalAdvance(text));
				int nextMarkerX = markerPos(m_xMapper->srcToDst(nextValue), i + 1, stepCount);
				int nextTextX = textPos(nextMarkerX, i + 1, stepCount, fm.horizontalAdvance(text));
				int textWidth = fm.horizontalAdvance(text+"M");
#else
				int textX = textPos(markerX, i, stepCount, fm.width(text));
				int nextMarkerX = markerPos(m_xMapper->srcToDst(nextValue), i + 1, stepCount);
				int nextTextX = textPos(nextMarkerX, i + 1, stepCount, fm.width(text));
				int textWidth = fm.width(text + "M");
#endif
				overlap = (textX + textWidth) >= nextTextX;
			}
			if (overlap)
				stepCount /= 2;
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
		double value = m_xTickBounds[0] + static_cast<double>(i) * stepWidth;
		if (value < startXVal)
			continue;
		else if (value > endXVal)
			break;
		QString text = xAxisTickMarkLabel(value, stepWidth);
		int markerX = markerPos(m_xMapper->srcToDst(value), i, stepCount);
		painter.drawLine(markerX, TickWidth, markerX, -1);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int textX = textPos(markerX, i, stepCount, fm.horizontalAdvance(text));
#else
		int textX = textPos(markerX, i, stepCount, fm.width(text));
#endif
		int textY = fm.height() + TextAxisDistance;
		painter.translate(textX, textY);
		painter.drawText(0, 0, text);
		painter.translate(-textX, -textY);
	}

	//draw the x axis
	painter.setPen(QWidget::palette().color(QPalette::Text));
	painter.drawLine(-m_translationX, -1, -m_translationX + activeWidth(), -1);
	if (m_drawXAxisAtZero && std::abs(-1.0-m_yMapper->srcToDst(0)) > 5) // if axis at bottom is at least 5 pixels away from zero point, draw additional line
		painter.drawLine(-m_translationX, -m_yMapper->srcToDst(0), -m_translationX + activeWidth(), -m_yMapper->srcToDst(0));

	if (m_showXAxisLabel)
	{
		//write the x axis label
		QPointF textPos(
			m_captionPosition.testFlag(Qt::AlignCenter) ?
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
				/* Center */ (int)(activeWidth() * 0.5 - m_translationX - (0.5*fm.horizontalAdvance(m_xCaption)))
#else
				/* Center */ (int)(activeWidth() * 0.5 - m_translationX - (0.5*fm.width(m_xCaption)))
#endif
				/* Left   */ : 0 ,
			m_captionPosition.testFlag(Qt::AlignBottom) ?
				/* Bottom */ bottomMargin() - fm.descent() - 1 :
				/* Top (of chart) */ -geometry().height() + bottomMargin() + m_fontHeight
		);
		painter.drawText(textPos, m_xCaption);
	}
}

void iAChartWidget::drawYAxis(QPainter &painter)
{
	if (leftMargin() <= 0)
	{
		return;
	}
	painter.save();
	painter.translate(-m_translationX, 0);
	QFontMetrics fm = painter.fontMetrics();
	int aheight = activeHeight() - 1;
	painter.setPen(QWidget::palette().color(QPalette::Text));

	// at most, make Y_AXIS_STEPS, but reduce to number actually fitting in current height:
	int stepNumber = std::min(AxisTicksYMax, static_cast<int>(aheight / (m_fontHeight*1.1)));
	stepNumber = std::max(1, stepNumber);	// make sure there's at least 2 steps
	const double step = 1.0 / (stepNumber * m_yZoom);

	for (int i = 0; i <= stepNumber; ++i)
	{
		double pos = step * i;
		int y = -static_cast<int>(pos * aheight * m_yZoom) - 1;
		double yValue = m_yMapper->dstToSrc(-y-1);
		QString text = dblToStringWithUnits(yValue);
		painter.drawLine(static_cast<int>(-TickWidth), y, 0, y);	// indicator line
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		painter.drawText( - ( fm.horizontalAdvance(text) + TickWidth),
#else
		painter.drawText( - ( fm.width(text) + TickWidth),
#endif
			(i == stepNumber) ? y + 0.75*m_fontHeight // write the text top aligned to the indicator line
			: y + 0.25*m_fontHeight                   // write the text centered to the indicator line
			, text);
	}
	painter.drawLine(0, -1, 0, -(int)(aheight*m_yZoom));
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
	m_xTickBounds[0] = valMin;
	m_xTickBounds[1] = valMax;
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
		return;
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
		return;
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
		m_xTickBounds[0] = (startPlot != 0) ? m_xTickBounds[0] : std::numeric_limits<double>::max();
		m_xTickBounds[1] = (startPlot != 0) ? m_xTickBounds[1] : std::numeric_limits<double>::lowest();
		m_maxXAxisSteps = 0;
		for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
		{
			auto d = m_plots[curPlot]->data();
			m_xBounds[0] = std::min(m_xBounds[0], d->xBounds()[0] - ((d->valueType() == Discrete) ? 0.5 : 0) );
			m_xBounds[1] = std::max(m_xBounds[1], d->xBounds()[1] + ((d->valueType() == Discrete) ? 0.5 : 0) );
			m_xTickBounds[0] = std::min(m_xTickBounds[0], d->xBounds()[0]);
			m_xTickBounds[1] = std::max(m_xTickBounds[1], d->xBounds()[1]);
			m_maxXAxisSteps = std::max(m_maxXAxisSteps, d->numBin() );
		}
		ensureNonZeroRange(m_xBounds);
		ensureNonZeroRange(m_xTickBounds);
	}
}

void iAChartWidget::updateBounds(size_t startPlot)
{
	updateXBounds(startPlot);
	updateYBounds(startPlot);
}

void iAChartWidget::drawBackground(QPainter &painter)
{
	if (!m_bgColor.isValid())
		m_bgColor = QWidget::palette().color(QWidget::backgroundRole());
	painter.fillRect( rect(), m_bgColor );
}

void iAChartWidget::resetView()
{
	m_translationX = 0;
	m_xZoom = 1.0;
	m_yZoom = 1.0;
	emit xAxisChanged();
	update();
}


// @{ TODO: these two methods need to be made plot-specific!
long iAChartWidget::screenX2DataBin(int x) const
{
	if (m_plots.empty())
		return x;
	double numBin = m_plots[0]->data()->numBin();
	double diagX = static_cast<double>(x - m_translationX - leftMargin()) * numBin / (activeWidth() * m_xZoom);
	diagX = clamp(0.0, numBin, diagX);
	return static_cast<long>(round(diagX));
}

int iAChartWidget::dataBin2ScreenX(long x) const
{
	if (m_plots.empty())
		return x;
	double numBin = m_plots[0]->data()->numBin();
	double screenX = static_cast<double>(x) * activeWidth() * m_xZoom / (numBin);
	screenX = clamp(0.0, activeWidth()*m_xZoom, screenX);
	return static_cast<int>(round(screenX));
}

bool iAChartWidget::isContextMenuVisible() const
{
	return m_contextMenuVisible;
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
		return ZoomXMaxEmpty;
	double numBin = m_plots[0]->data()->numBin();
	return std::max(std::min(ZoomXMax, numBin), 1.0);
}

void iAChartWidget::setYMappingMode(AxisMappingType drawMode)
{
	if (m_yMappingMode == drawMode)
		return;
	m_yMappingMode = drawMode;
	createMappers();
}

void iAChartWidget::setCaptionPosition(QFlags<Qt::AlignmentFlag> captionPosition)
{
	m_captionPosition = captionPosition;
}

void iAChartWidget::setShowXAxisLabel(bool show)
{
	m_showXAxisLabel = show;
}

void iAChartWidget::addPlot(QSharedPointer<iAPlot> plot)
{
	assert(plot);
	if (!plot)
		return;
	m_plots.push_back(plot);
	updateBounds(m_plots.size()-1);
}

void iAChartWidget::removePlot(QSharedPointer<iAPlot> plot)
{
	if (!plot)
		return;
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

bool iAChartWidget::isDrawnDiscrete() const
{
	for (auto plot : m_plots)
		if (!((plot->data()->valueType() == Discrete && (xRange() <= plot->data()->numBin()))
			  || plot->data()->valueType() == Categorical))
			return false;
	return !m_plots.empty();
}

void iAChartWidget::addImageOverlay(QSharedPointer<QImage> imgOverlay)
{
	m_overlays.push_back(imgOverlay);
}

void iAChartWidget::removeImageOverlay(QImage * imgOverlay)
{
	for (int i = 0; i < m_overlays.size(); ++i)
	{
		if (m_overlays.at(i).data() == imgOverlay)
		{
			m_overlays.removeAt(i);
			return;
		}
	}
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
		painter.drawText(QRect(-m_translationX, -m_translationY, activeWidth(), -activeHeight()), Qt::AlignCenter, "Chart not (yet) available.");
		painter.scale(1, -1);
		return;
	}
	double xStart = visibleXStart(), xEnd = visibleXEnd();
	for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
	{
		if ((*it)->visible())
		{
			size_t plotNumBin = (*it)->data()->numBin();
			double plotXBounds[2] = {(*it)->data()->xBounds()[0], (*it)->data()->xBounds()[1]};
			ensureNonZeroRange(plotXBounds);
			double plotVisXBounds[2] = {
				clamp((*it)->data()->xBounds()[0], (*it)->data()->xBounds()[1], xStart),
				clamp((*it)->data()->xBounds()[0], (*it)->data()->xBounds()[1], xEnd)
			};
			ensureNonZeroRange(plotVisXBounds);
			double plotStepWidth = (plotXBounds[1] - plotXBounds[0]
				+ (((*it)->data()->valueType() == Discrete)?1:0) ) / m_plots[0]->data()->numBin();
			size_t plotStartBin = static_cast<size_t>(clamp(0.0, static_cast<double>(plotNumBin - 1), (plotVisXBounds[0] - (*it)->data()->xBounds()[0]) / plotStepWidth - 1));
			size_t plotEndBin = static_cast<size_t>(clamp(0.0, static_cast<double>(plotNumBin - 1), (plotVisXBounds[1] - (*it)->data()->xBounds()[0]) / plotStepWidth + 1));
			double plotPixelBinWidth = m_xMapper->srcToDst(xBounds()[0] + plotStepWidth);
			iALinearMapper plotXMapper;
			if ((*it)->data()->valueType() == Continuous)
				plotXMapper.update(-1, plotNumBin + 1, m_xMapper->srcToDst((*it)->data()->xBounds()[0] - plotStepWidth), m_xMapper->srcToDst((*it)->data()->xBounds()[1] + plotStepWidth));
			else
				plotXMapper.update(-1, plotNumBin, m_xMapper->srcToDst((*it)->data()->xBounds()[0] - plotStepWidth), m_xMapper->srcToDst((*it)->data()->xBounds()[1] + plotStepWidth));
			(*it)->draw(painter, plotPixelBinWidth, plotStartBin, plotEndBin, plotXMapper, *m_yMapper.data());
		}
	}
}

bool iAChartWidget::event(QEvent *event)
{
	if (event->type() != QEvent::ToolTip)
		return iAQGLWidget::event(event);

	if (m_plots.empty() || !m_showTooltip)
	{
		QToolTip::hideText();
		event->ignore();
	}
	QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
	showDataTooltip(helpEvent);
	return true;
}

void iAChartWidget::showDataTooltip(QHelpEvent *event)
{
	if (m_plots.empty())
		return;
	size_t numBin = m_plots[0]->data()->numBin();
	assert(numBin > 0);
	int xPos = clamp(0, geometry().width() - 1, event->x());
	size_t nthBin = (((xPos - m_translationX - leftMargin()) * numBin) / (activeWidth())) / m_xZoom;
	nthBin = clamp(static_cast<size_t>(0), numBin, nthBin);
	if (xPos == geometry().width() - 1)
		nthBin = static_cast<int>(numBin) - 1;
	QString toolTip;
	double stepWidth = numBin >= 1 ? m_plots[0]->data()->binStart(1) - m_plots[0]->data()->binStart(0) : 0;
	double binStart = m_plots[0]->data()->binStart(nthBin);
	if (isDrawnDiscrete())
		binStart = static_cast<int>(binStart);
	if (m_yCaption.isEmpty())
		toolTip = QString("%1: ").arg(xAxisTickMarkLabel(binStart, stepWidth));
	else
		toolTip = QString("%1: %2\n%3: ").arg(m_xCaption).arg(xAxisTickMarkLabel(binStart, stepWidth)).arg(m_yCaption);
	bool more = false;
	const int MaxToolTipDataCount = 5;
	int curTooltipDataCount = 1;
	for (auto plot : m_plots)
	{
		if (!plot->data() || !plot->data()->rawData())
			continue;
		if (more)
			toolTip += ", ";
		else
			more = true;
		toolTip += QString::number(plot->data()->rawData()[nthBin], 'g', 15);
		++curTooltipDataCount;
		if (curTooltipDataCount > MaxToolTipDataCount)
		{
			toolTip += "...";
			break;
		}
	}
	QToolTip::showText(event->globalPos(), toolTip, this);
}

void iAChartWidget::changeMode(int newMode, QMouseEvent *event)
{
	switch(newMode)
	{
	case MOVE_VIEW_MODE:
		m_dragStartPosX = event->x();
		m_dragStartPosY = event->y();
		m_mode = MOVE_VIEW_MODE;
		break;
	default:
		m_mode = newMode;
		break;
	}
}

void iAChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (m_selectionBand->isVisible())
		{
			m_selectionBand->hide();
			QRectF diagramRect;
			QRectF selectionRect(m_selectionBand->geometry());     // height-y because we are drawing reversed from actual y direction
			diagramRect.setTop(    yMapper().dstToSrc(activeHeight() - selectionRect.bottom()) );
			diagramRect.setBottom( yMapper().dstToSrc(activeHeight() - selectionRect.top()   ) );
			diagramRect.setLeft(   screenX2DataBin(selectionRect.left()  ) );
			diagramRect.setRight(  screenX2DataBin(selectionRect.right() ) );
			diagramRect = diagramRect.normalized();
			if (diagramRect.top() < yBounds()[0])
				diagramRect.setTop(yBounds()[0]);
			if (diagramRect.bottom() > yBounds()[1])
				diagramRect.setBottom(yBounds()[1]);
			m_selectedPlots.clear();
			double yMin = diagramRect.top(), yMax = diagramRect.bottom();
			for (size_t plotIdx=0; plotIdx<m_plots.size(); ++plotIdx)
			{
				if (!m_plots[plotIdx]->visible())
					continue;
				for (int bin=diagramRect.left(); bin <= diagramRect.right(); ++bin)
				{
					double binYValue = m_plots[plotIdx]->data()->rawData()[bin];
					if (yMin < binYValue && binYValue < yMax)
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
			m_zoomYPos = event->y();
			m_yZoomStart = m_yZoom;
			changeMode(Y_ZOOM_MODE, event);
		}
		else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
			((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
		{
			m_zoomXPos = event->x();
			m_zoomYPos = event->y();
			m_xZoomStart = m_xZoom;
			changeMode(X_ZOOM_MODE, event);
		}
		else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
		{
			m_translationStartX = m_translationX;
			m_translationStartY = m_translationY;
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
	case Qt::MidButton:
		m_translationStartX = m_translationX;
		m_translationStartY = m_translationY;
		changeMode(MOVE_VIEW_MODE, event);
		break;
	default:
		break;
	}
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
		m_translationX = clamp(-static_cast<int>(activeWidth() * (m_xZoom-1)), 0,
			m_translationStartX + event->x() - m_dragStartPosX);
		emit xAxisChanged();
		m_translationY = m_translationStartY + event->y() - m_dragStartPosY;
		m_translationY = clamp(static_cast<int>(-(geometry().height() * m_yZoom - geometry().height())),
				static_cast<int>(geometry().height() * m_yZoom - geometry().height()), m_translationY);
		update();
		break;
	case X_ZOOM_MODE:
		zoomAlongX(((m_zoomYPos-event->y())/2.0)+ m_xZoomStart, m_zoomXPos, false);
		update();
		break;
	case Y_ZOOM_MODE:
		{
			int diff = (m_zoomYPos-event->y())/2.0;
			if (diff < 0)
				zoomAlongY(-pow(ZoomYStep,-diff)+ m_yZoomStart, false);
			else
				zoomAlongY(pow(ZoomYStep,diff)+ m_yZoomStart, false);
			update();
		}
		break;
	}
}

QImage iAChartWidget::drawOffscreen()
{
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
		qFatal("Cannot create the requested OpenGL context!");
	context.makeCurrent(&window);
	const QSize drawRectSize(width(), height());
	QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setSamples(4);
	fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	QOpenGLFramebufferObject fbo(drawRectSize, fboFormat);
	fbo.bind();
	QOpenGLPaintDevice device(drawRectSize);
	QPainter p;
	p.begin(&device);
	drawAll(p);
	p.end();
	fbo.release();
	QImage image = fbo.toImage();
	context.doneCurrent();
	return image;
}

void iAChartWidget::setBackgroundColor(QColor const & color)
{
	m_bgColor = color;
	update();
}

void iAChartWidget::paintGL()
{
	QPainter p(this);
	drawAll(p);
}

void iAChartWidget::drawAll(QPainter & painter)
{
	painter.setRenderHint(QPainter::Antialiasing);
	drawBackground(painter);
	if (activeWidth() <= 1 || activeHeight() <= 1)
		return;
	if (!m_xMapper || !m_yMapper)
		createMappers();
	m_xMapper->update(m_xBounds[0], m_xBounds[1], 0, m_xZoom*(activeWidth()-1));
	m_yMapper->update(m_yMappingMode == Logarithmic && m_yBounds[0] <= 0 ? 1 : m_yBounds[0], m_yBounds[1], 0, m_yZoom*(activeHeight()-1));
	QFontMetrics fm = painter.fontMetrics();
	m_fontHeight = fm.height();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	m_yMaxTickLabelWidth = fm.horizontalAdvance("4.44M");
#else
	m_yMaxTickLabelWidth = fm.width("4.44M");
#endif
	painter.translate(m_translationX + leftMargin(), -bottomMargin());
	drawImageOverlays(painter);
	//change the origin of the window to left bottom
	painter.translate(0, geometry().height());
	painter.scale(1, -1);

	drawPlots(painter);
	for (double x : m_xMarker.keys())
	{
		QColor color = m_xMarker[x];
		painter.setPen(color);
		QLine line;
		QRect diagram = geometry();
		double pos = m_xMapper->srcToDst(x);
		line.setP1(QPoint(pos, 0));
		line.setP2(QPoint(pos, diagram.height() - bottomMargin()));
		painter.drawLine(line);
	}

	drawAfterPlots(painter);

	painter.scale(1, -1);
	painter.setRenderHint(QPainter::Antialiasing, false);
	drawAxes(painter);
}

void iAChartWidget::addXMarker(double xPos, QColor const & color)
{
	m_xMarker.insert(xPos, color);
}

void iAChartWidget::removeXMarker(double xPos)
{
	m_xMarker.remove(xPos);
}

void iAChartWidget::clearMarkers()
{
	m_xMarker.clear();
}

void iAChartWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Alt ||
		event->key() == Qt::Key_AltGr ||
		event->key() == Qt::Key_Escape)
		m_contextMenuVisible = false;
}

void iAChartWidget::addContextMenuEntries(QMenu* /*contextMenu*/)
{}

void iAChartWidget::contextMenuEvent(QContextMenuEvent *event)
{
	m_contextPos = event->pos();
	m_contextMenu->clear();
	m_contextMenu->addAction(QIcon(":/images/resetView.png"), tr("Reset histogram view"), this, SLOT(resetView()));
	QAction *showTooltipAction = new QAction(tr("Show histogram coordinates"), this);
	showTooltipAction->setCheckable(true);
	showTooltipAction->setChecked(m_showTooltip);
	connect(showTooltipAction, SIGNAL(toggled(bool)), this, SLOT(showTooltip(bool)));
	m_contextMenu->addAction(showTooltipAction);
	m_contextMenu->addAction(QIcon(":/images/save.png"), tr("Export histogram data"), this, SLOT(exportData()));
	m_contextMenu->addSeparator();
	addContextMenuEntries(m_contextMenu);
	m_contextMenuVisible = true;
	m_contextMenu->exec(event->globalPos());
}

void iAChartWidget::exportData()
{
	// TODO: Allow choosing which plot to export!
	if (m_plots.empty())
		return;
	QString filePath = ""; //(activeChild) ? activeChild->getFilePath() : "";
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("Save File"),
		filePath, tr("CSV (*.csv)")
	);
	if (fileName.isEmpty())
	{
		return;
	}
	std::ofstream out( getLocalEncodingFileName(fileName));
	out << tr("Start of Bin").toStdString();
	for (size_t p = 0; p < m_plots.size(); ++p)
	{
		out << "," << QString("%1%2").arg(m_yCaption).arg(p).toStdString();
	}
	out << std::endl;
	for (size_t b = 0; b < m_plots[0]->data()->numBin(); ++b)
	{
		out << QString::number(m_plots[0]->data()->binStart(b), 'g', 15).toStdString();
		for (int p = 0; p < m_plots.size(); ++p)
		{
			out << "," << QString::number(m_plots[p]->data()->rawData()[b], 'g', 15).toStdString();
		}
		out << std::endl;
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
