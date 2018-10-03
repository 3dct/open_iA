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

#include <vtkMath.h>

#include <QAction>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QSurfaceFormat>
#endif
#include <QToolTip>
#include <QWheelEvent>

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
	const int CategoricalTextRotation = 15;
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

	int markerPos(int x, int step, int stepCount)
	{
		if (step == stepCount) --x;
		return x;
	}

	int textPos(int markerX, int step, int stepNr, int textWidth)
	{
		return (step == 0)
			? markerX					// right aligned to indicator line
			: (step < stepNr)
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
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QOpenGLWidget(parent),
#else
	QGLWidget(parent),
#endif
	xCaption(xLabel),
	yCaption(yLabel),
	yZoom(1.0),
	yZoomStart(1.0),
	xZoom(1.0),
	xZoomStart(1.0),
	translationX(0),
	translationY(0),
	translationStartX(0),
	translationStartY( 0 ),
	mode(NO_MODE),
	m_contextMenuVisible(false),
	m_customXBounds(false),
	m_customYBounds(false),
	m_maxXAxisSteps(AxisTicksXDefault),
	m_yMappingMode(Linear),
	m_contextMenu(new QMenu(this)),
	m_showTooltip(true),
	m_showXAxisLabel(true),
	m_captionPosition(Qt::AlignCenter | Qt::AlignBottom),
	m_fontHeight(0),
	m_yMaxTickLabelWidth(0)
{
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QSurfaceFormat fmt = format();
	fmt.setSamples(8);
	setFormat(fmt);
#endif
	updateBounds();
	setMouseTracking(true);
	setFocusPolicy(Qt::WheelFocus);
	setAutoFillBackground(false);
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
			yZoom *= ZoomYStep;
		}
		else
		{
			yZoom /= ZoomYStep;
		}
	}
	else
	{
		yZoom = value;
	}
	yZoom = clamp(ZoomYMin, ZoomYMax, yZoom);
}

void iAChartWidget::zoomAlongX(double value, int x, bool deltaMode)
{
	int xZoomBefore = xZoom;
	int translationXBefore = translationX;
	// don't do anything if we're already at the limit
	if ( (deltaMode &&  (value < 0    && xZoom == 1.0) || (value > 0           && xZoom == maxXZoom())) ||
		 (!deltaMode && (value <= 1.0 && xZoom == 1.0) || (value >= maxXZoom() && xZoom == maxXZoom())) )
	{
		return;
	}
	int absoluteX = x-translationX-leftMargin();
	double absoluteXRatio = (double)absoluteX/((activeWidth()-1)*xZoom);
	if (deltaMode)
		if (value /* = delta */ > 0)
			xZoom *= ZoomXStep;
		else
			xZoom /= ZoomXStep;
	else
		xZoom = value;

	xZoom = clamp(ZoomXMin, maxXZoom(), xZoom);

	int absXAfterZoom = (int)(activeWidth()*xZoom*absoluteXRatio);

	translationX = clamp(-static_cast<int>(activeWidth() * (xZoom-1)), 0,
		-absXAfterZoom +x -leftMargin());

	if (xZoomBefore != xZoom || translationXBefore != translationX)
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
	xCaption = caption;
}

void iAChartWidget::setYCaption(QString const & caption)
{
	yCaption = caption;
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
	return (yCaption == "") ? 0 :
		MarginLeft + m_fontHeight + m_yMaxTickLabelWidth + TickWidth;
}

iAPlotData::DataType iAChartWidget::minYDataValue(size_t startPlot) const
{
	iAPlotData::DataType minVal = std::numeric_limits<iAPlotData::DataType>::max();
	for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
		minVal = std::min(m_plots[curPlot]->data()->YBounds()[0], minVal);
	return minVal;
}

iAPlotData::DataType iAChartWidget::maxYDataValue(size_t startPlot) const
{
	iAPlotData::DataType maxVal = std::numeric_limits<iAPlotData::DataType>::lowest();
	for (size_t curPlot = std::max(static_cast<size_t>(0), startPlot); curPlot < m_plots.size(); ++curPlot)
		maxVal = std::max(m_plots[curPlot]->data()->YBounds()[1], maxVal);
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
	m_xMapper = QSharedPointer<iAMapper>(new iALinearMapper(m_xBounds[0], m_xBounds[1], 0, (activeWidth() - 1)*xZoom));
	if (m_yMappingMode == Linear)
		m_yMapper = QSharedPointer<iAMapper>(new iALinearMapper(m_yBounds[0], m_yBounds[1], 0, (activeHeight()-1)*yZoom));
	else
	{
		m_yMapper = QSharedPointer<iAMapper>(new iALogarithmicMapper(m_yBounds[0] > 0 ? m_yBounds[0] : 1, m_yBounds[1], 0, (activeHeight() - 1)*yZoom));
		if (m_yBounds[0] <= 0)
		{
			DEBUG_LOG(QString("Invalid y bounds in chart for logarithmic mapping: minimum=%1 is <= 0, using 1 instead.").arg(m_yBounds[0]));
		}
	}
}

void iAChartWidget::drawImageOverlays(QPainter& painter)
{
	QRect targetRect = geometry();
	int yTranslate = -(yZoom - 1) * (targetRect.height());
	targetRect.setHeight(targetRect.height() - targetRect.top() - 1);
	targetRect.setWidth((targetRect.width() - leftMargin()) * xZoom);
	targetRect.setTop(targetRect.top() + yTranslate);
	targetRect.setLeft(0);
	for (int i = 0; i < m_overlays.size(); ++i)
	{
		painter.drawImage(targetRect, *(m_overlays[i].data()), m_overlays[i]->rect());
	}
}

void iAChartWidget::drawAfterPlots(QPainter& painter)
{}

QString iAChartWidget::getXAxisTickMarkLabel(double value, double stepWidth)
{
	int placesBeforeComma = requiredDigits(value);
	int placesAfterComma = (stepWidth < 10) ? requiredDigits(10 / stepWidth) : 0;
	if ((!m_plots.empty() && m_plots[0]->data()->GetRangeType() == Continuous) || placesAfterComma > 1)
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
		return (m_plots[0]->data()->GetRangeType() == Categorical);
	else
		return false;
}

double iAChartWidget::visibleXStart() const
{
	return xBounds()[0] + (((static_cast<double>(-translationX)) / (activeWidth()*xZoom)) * xRange());
}

double iAChartWidget::visibleXEnd() const
{
	double visibleRange = xRange() / xZoom;
	return visibleXStart() + visibleRange;
}

void iAChartWidget::drawXAxis(QPainter &painter)
{
	painter.setPen(QWidget::palette().color(QPalette::Text));
	const int MINIMUM_MARGIN = 8;
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
				QString text = getXAxisTickMarkLabel(value, stepWidth);
				int markerX = markerPos(m_xMapper->srcToDst(value), i, stepCount);
				int textX = textPos(markerX, i, stepCount, fm.width(text));
				int nextMarkerX = markerPos(m_xMapper->srcToDst(nextValue), i + 1, stepCount);
				int nextTextX = textPos(nextMarkerX, i + 1, stepCount, fm.width(text));
				int textWidth = fm.width(text) + fm.width("M");
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
	for (int i = 0; i <= stepCount; ++i)
	{
		double value = m_xTickBounds[0] + static_cast<double>(i) * stepWidth;
		if (value < startXVal)
			continue;
		else if (value > endXVal)
			break;
		QString text = getXAxisTickMarkLabel(value, stepWidth);
		int markerX = markerPos(m_xMapper->srcToDst(value), i, stepCount);
		painter.drawLine(markerX, TickWidth, markerX, -1);
		int textX = textPos(markerX, i, stepCount, fm.width(text));
		int textY = fm.height() + TextAxisDistance;
		painter.translate(textX, textY);
		painter.drawText(0, 0, text);
		painter.translate(-textX, -textY);
	}

	//draw the x axis
	painter.setPen(QWidget::palette().color(QPalette::Text));
	painter.drawLine(-translationX, -1, -translationX + activeWidth(), -1);

	if (m_showXAxisLabel)
	{
		//write the x axis label
		QPointF textPos(
			m_captionPosition.testFlag(Qt::AlignCenter) ?
				/* Center */ (int)(activeWidth() * 0.5 - translationX - (0.5*fm.width(xCaption)))
				/* Left   */ : 0 ,
			m_captionPosition.testFlag(Qt::AlignBottom) ?
				/* Bottom */ bottomMargin() - fm.descent() - 1 :
				/* Top (of chart) */ -geometry().height() + bottomMargin() + m_fontHeight
		);
		painter.drawText(textPos, xCaption);
	}
}

void iAChartWidget::drawYAxis(QPainter &painter)
{
	if (leftMargin() <= 0)
	{
		return;
	}
	painter.save();
	painter.translate(-translationX, 0);
	QFontMetrics fm = painter.fontMetrics();

	int aheight = activeHeight() - 1;
	painter.fillRect(QRect(0, bottomMargin(), -leftMargin(), -(aheight + bottomMargin() + 1)),
		QBrush(QWidget::palette().color(QWidget::backgroundRole())));
	painter.setPen(QWidget::palette().color(QPalette::Text));

	// at most, make Y_AXIS_STEPS, but reduce to number actually fitting in current height:
	int stepNumber = std::min(AxisTicksYMax, static_cast<int>(aheight / (m_fontHeight*1.1)));
	stepNumber = std::max(1, stepNumber);	// make sure there's at least 2 steps
	const double step = 1.0 / (stepNumber * yZoom);
	double logMax = LogFunc(static_cast<double>(m_yBounds[1]));

	for (int i = 0; i <= stepNumber; ++i)
	{
		double pos = step * i;
		int y = -static_cast<int>(pos * aheight * yZoom) - 1;
		double yValue = m_yMapper->dstToSrc(-y-1);
		QString text = DblToStringWithUnits(yValue);
		painter.drawLine(static_cast<int>(-TickWidth), y, 0, y);	// indicator line
		painter.drawText( - ( fm.width(text) + TickWidth),
			(i == stepNumber) ? y + 0.75*m_fontHeight // write the text top aligned to the indicator line
			: y + 0.25*m_fontHeight                   // write the text centered to the indicator line
			, text);
	}
	painter.drawLine(0, -1, 0, -(int)(aheight*yZoom));
	//write the y axis label
	painter.save();
	painter.rotate(-90);
	QPointF textPos(
		aheight*0.5 - 0.5*fm.width(yCaption),
		-leftMargin() + m_fontHeight - 5);
	painter.drawText(textPos, yCaption);
	painter.restore();
	painter.restore();
}

void iAChartWidget::setXBounds(double valMin, double valMax)
{
	m_customXBounds = true;
	m_xBounds[0] = valMin;
	m_xBounds[1] = valMax;
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
			m_xBounds[0] = std::min(m_xBounds[0], d->XBounds()[0] - ((d->GetRangeType() == Discrete) ? 0.5 : 0) );
			m_xBounds[1] = std::max(m_xBounds[1], d->XBounds()[1] + ((d->GetRangeType() == Discrete) ? 0.5 : 0) );
			m_xTickBounds[0] = std::min(m_xTickBounds[0], d->XBounds()[0]);
			m_xTickBounds[1] = std::max(m_xTickBounds[1], d->XBounds()[1]);
			m_maxXAxisSteps = std::max(m_maxXAxisSteps, d->GetNumBin() );
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
	painter.fillRect( rect(), QWidget::palette().color(QWidget::backgroundRole()));
}

void iAChartWidget::resetView()
{
	translationX = 0;
	xZoom = 1.0;
	yZoom = 1.0;
	emit xAxisChanged();
	update();
}


// @{ TODO: these two methods need to be made plot-specific!
long iAChartWidget::screenX2DataBin(int x) const
{
	if (m_plots.empty())
		return x;
	double numBin = m_plots[0]->data()->GetNumBin();
	double diagX = static_cast<double>(x - translationX - leftMargin()) * numBin / (activeWidth() * xZoom);
	diagX = clamp(0.0, numBin, diagX);
	return static_cast<long>(round(diagX));
}

int iAChartWidget::dataBin2ScreenX(long x) const
{
	if (m_plots.empty())
		return x;
	double numBin = m_plots[0]->data()->GetNumBin();
	double screenX = static_cast<double>(x) * activeWidth() * xZoom / (numBin);
	screenX = clamp(0.0, activeWidth()*xZoom, screenX);
	return static_cast<int>(round(screenX));
}
//! @}

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
	double numBin = m_plots[0]->data()->GetNumBin();
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

std::vector<QSharedPointer<iAPlot> > const & iAChartWidget::plots()
{
	return m_plots;
}

bool iAChartWidget::isDrawnDiscrete() const
{
	for (auto plot : m_plots)
		if (!((plot->data()->GetRangeType() == Discrete && (xRange() <= plot->data()->GetNumBin()))
			  || plot->data()->GetRangeType() == Categorical))
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

void iAChartWidget::drawPlots(QPainter &painter)
{
	if (m_plots.empty())
	{
		painter.scale(1, -1);
		painter.drawText(QRect(-translationX, -translationY, activeWidth(), -activeHeight()), Qt::AlignCenter, "Chart not (yet) available.");
		painter.scale(1, -1);
		return;
	}
	double xStart = visibleXStart(), xEnd = visibleXEnd();
	for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
	{
		if ((*it)->visible())
		{
			size_t plotNumBin = (*it)->data()->GetNumBin();
			double plotXBounds[2] = {(*it)->data()->XBounds()[0], (*it)->data()->XBounds()[1]};
			ensureNonZeroRange(plotXBounds);
			double plotVisXBounds[2] = {
				clamp((*it)->data()->XBounds()[0], (*it)->data()->XBounds()[1], xStart),
				clamp((*it)->data()->XBounds()[0], (*it)->data()->XBounds()[1], xEnd)
			};
			ensureNonZeroRange(plotVisXBounds);
			double plotStepWidth = (plotXBounds[1] - plotXBounds[0]
				+ (((*it)->data()->GetRangeType() == Discrete)?1:0) ) / m_plots[0]->data()->GetNumBin();
			size_t plotStartBin = static_cast<size_t>(clamp(0.0, static_cast<double>(plotNumBin - 1), (plotVisXBounds[0] - (*it)->data()->XBounds()[0]) / plotStepWidth - 1));
			size_t plotEndBin = static_cast<size_t>(clamp(0.0, static_cast<double>(plotNumBin - 1), (plotVisXBounds[1] - (*it)->data()->XBounds()[0]) / plotStepWidth + 1));
			double plotPixelBinWidth = m_xMapper->srcToDst(xBounds()[0] + plotStepWidth);
			iALinearMapper plotXMapper;
			if ((*it)->data()->GetRangeType() == Continuous)
				plotXMapper.update(-1, plotNumBin + 1, m_xMapper->srcToDst((*it)->data()->XBounds()[0] - plotStepWidth), m_xMapper->srcToDst((*it)->data()->XBounds()[1] + plotStepWidth));
			else
				plotXMapper.update(-1, plotNumBin, m_xMapper->srcToDst((*it)->data()->XBounds()[0] - plotStepWidth), m_xMapper->srcToDst((*it)->data()->XBounds()[1] + plotStepWidth));
			(*it)->draw(painter, plotPixelBinWidth, plotStartBin, plotEndBin, plotXMapper, *m_yMapper.data());
		}
	}
}

void iAChartWidget::showDataTooltip(QMouseEvent *event)
{
	if (m_plots.empty() || !m_showTooltip)
		return;
	int xPos = clamp(0, geometry().width() - 1, event->x());
	size_t numBin = m_plots[0]->data()->GetNumBin();
	assert(numBin > 0);
	int nthBin = static_cast<int>((((xPos - translationX - leftMargin()) * numBin) / (activeWidth())) / xZoom);
	nthBin = clamp(0, static_cast<int>(numBin), nthBin);
	if (xPos == geometry().width() - 1)
		nthBin = static_cast<int>(numBin) - 1;
	QString toolTip;
	double stepWidth = numBin >= 1 ? m_plots[0]->data()->GetBinStart(1) - m_plots[0]->data()->GetBinStart(0) : 0;
	double binStart = m_plots[0]->data()->GetBinStart(nthBin);
	if (isDrawnDiscrete())
		binStart = static_cast<int>(binStart);
	if (yCaption.isEmpty())
		toolTip = QString("%1: ").arg(getXAxisTickMarkLabel(binStart, stepWidth));
	else
		toolTip = QString("%1: %2\n%3: ").arg(xCaption).arg(getXAxisTickMarkLabel(binStart, stepWidth)).arg(yCaption);
	bool more = false;
	for (auto plot : m_plots)
	{
		auto data = plot->data();
		if (!data || !data->GetRawData())
			continue;
		if (more)
			toolTip += ", ";
		else
			more = true;
		toolTip += QString::number(data->GetRawData()[nthBin], 'g', 15);
	}
	QToolTip::showText(event->globalPos(), toolTip, this);
}

void iAChartWidget::changeMode(int newMode, QMouseEvent *event)
{
	switch(newMode)
	{
	case MOVE_VIEW_MODE:
		dragStartPosX = event->x();
		dragStartPosY = event->y();
		mode = MOVE_VIEW_MODE;
		break;
	default:
		mode = newMode;
		break;
	}
}

void iAChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		update();
	this->mode = NO_MODE;
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
			zoomY = event->y();
			yZoomStart = yZoom;
			changeMode(Y_ZOOM_MODE, event);
		}
		else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
			((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
		{
			zoomX = event->x();
			zoomY = event->y();
			xZoomStart = xZoom;
			changeMode(X_ZOOM_MODE, event);
		}
		else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
		{
			translationStartX = translationX;
			translationStartY = translationY;
			changeMode(MOVE_VIEW_MODE, event);
		}
		break;
	case Qt::RightButton:
		update();
		break;
	case Qt::MidButton:
		translationStartX = translationX;
		translationStartY = translationY;
		changeMode(MOVE_VIEW_MODE, event);
		break;
	default:
		break;
	}
}

void iAChartWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch(mode)
	{
	case NO_MODE: /* do nothing */ break;
	case MOVE_VIEW_MODE:
		translationX = clamp(-static_cast<int>(activeWidth() * (xZoom-1)), 0,
			translationStartX + event->x() - dragStartPosX);
		emit xAxisChanged();
		translationY = translationStartY + event->y() - dragStartPosY;
		translationY = clamp(static_cast<int>(-(geometry().height() * yZoom - geometry().height())),
				static_cast<int>(geometry().height() * yZoom - geometry().height()), translationY);
		update();
		break;
	case X_ZOOM_MODE:
		zoomAlongX(((zoomY-event->y())/2.0)+xZoomStart, zoomX, false);
		update();
		break;
	case Y_ZOOM_MODE:
		{
			int diff = (zoomY-event->y())/2.0;
			if (diff < 0)
				zoomAlongY(-pow(ZoomYStep,-diff)+yZoomStart, false);
			else
				zoomAlongY(pow(ZoomYStep,diff)+yZoomStart, false);
			update();
		}
		break;
	}
	showDataTooltip(event);
}

void iAChartWidget::paintEvent(QPaintEvent * e)
{
	if (geometry().width() <= 1 || geometry().height() <= 1)
		return;
	QPainter painter(this);
	if (!m_xMapper || !m_yMapper)
		createMappers();
	m_xMapper->update(m_xBounds[0], m_xBounds[1], 0, xZoom*(activeWidth()-1));
	m_yMapper->update(m_yMappingMode == Linear || m_yBounds > 0 ? m_yBounds[0] : 1, m_yBounds[1], 0, yZoom*(activeHeight()-1));
	QFontMetrics fm = painter.fontMetrics();
	m_fontHeight = fm.height();
	m_yMaxTickLabelWidth = fm.width("4.44M");
	painter.setRenderHint(QPainter::Antialiasing);
	drawBackground(painter);
	painter.translate(translationX + leftMargin(), -bottomMargin());
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

void iAChartWidget::addContextMenuEntries(QMenu* contextMenu)
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
	std::ofstream out(fileName.toStdString());
	out << tr("Start of Bin").toStdString();
	for (int p = 0; p < m_plots.size(); ++p)
	{
		out << "," << QString("%1%2").arg(yCaption).arg(p).toStdString();
	}
	out << std::endl;
	for (int b = 0; b < m_plots[0]->data()->GetNumBin(); ++b)
	{
		out << QString::number(m_plots[0]->data()->GetBinStart(b), 'g', 15).toStdString();
		for (int p = 0; p < m_plots.size(); ++p)
		{
			out << "," << QString::number(m_plots[p]->data()->GetRawData()[b], 'g', 15).toStdString();
		}
		out << std::endl;
	}
	out.close();
}

void iAChartWidget::showTooltip(bool toggled)
{
	m_showTooltip = toggled;
}
