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
	const double MIN_X_ZOOM = 1.0;
	const double MAX_X_ZOOM = 2048;
	const double MIN_Y_ZOOM = 1.0;		// the y-axis is not adapted (yet) to work with zoom < 1.0
	const double MAX_Y_ZOOM = 32768;
	const double X_ZOOM_STEP = 1.5;
	const double Y_ZOOM_STEP = 1.5;
	const int CATEGORICAL_TEXT_ROTATION = 15;
	const int CATEGORICAL_FONT_SIZE = 7;
	const int LEFT_MARGIN = 5;
	const int BOTTOM_MARGIN = 5;
	const int Y_AXIS_STEPS = 5;
	const int TickWidth = 6;
	const size_t MAX_X_AXIS_STEPS = 32 * static_cast<size_t>(MAX_X_ZOOM);
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
	m_maxXAxisSteps(MAX_X_AXIS_STEPS),
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
			yZoom *= Y_ZOOM_STEP;
		}
		else
		{
			yZoom /= Y_ZOOM_STEP;
		}
	}
	else
	{
		yZoom = value;
	}
	yZoom = clamp(MIN_Y_ZOOM, MAX_Y_ZOOM, yZoom);
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
			xZoom *= X_ZOOM_STEP;
		else
			xZoom /= X_ZOOM_STEP;
	else
		xZoom = value;

	xZoom = clamp(MIN_X_ZOOM, maxXZoom(), xZoom);

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
	return BOTTOM_MARGIN + static_cast<int>((m_showXAxisLabel ? 2 : 1) * m_fontHeight);
}

int iAChartWidget::leftMargin() const
{
	return (yCaption == "") ? 0 :
		LEFT_MARGIN + m_fontHeight + m_yMaxTickLabelWidth + TickWidth;
}

iAPlotData::DataType iAChartWidget::getMaxYDataValue() const
{
	iAPlotData::DataType maxVal = std::numeric_limits<iAPlotData::DataType>::lowest();
	for (auto plot : m_plots)
		maxVal = std::max(plot->data()->YBounds()[1], maxVal);
	return maxVal;
}

iAPlotData::DataType const * iAChartWidget::yBounds() const
{
	return m_yBounds;
}

QSharedPointer<iAMapper> const iAChartWidget::yMapper() const
{
	return m_yMapper;
}

void iAChartWidget::createYConverter()
{
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

namespace
{
	int requiredDigits(double value)
	{
		return (value >= -1.0 && value < 1.0) ?
			1 : std::floor(std::log10(std::abs(value))) + 1;
	}

	double deg2rad(double const & number)
	{
		return number * vtkMath::Pi() / 180;
	}

	int markerPos(int step, int stepCount, int width, int offset)
	{
		int x = static_cast<int>(static_cast<double>(step) / stepCount * width) + offset;
		if (step == stepCount) x--;
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
}

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

void iAChartWidget::drawXAxis(QPainter &painter)
{
	painter.setPen(QWidget::palette().color(QPalette::Text));
	const int MINIMUM_MARGIN = 8;
	const int TextAxisDistance = 2;
	QFontMetrics fm = painter.fontMetrics();
	size_t stepCount = m_maxXAxisSteps;
	double stepWidth;
	// for discrete x axis variables, marker&caption should be in middle of value; MaxXAxisSteps() = numBin of plot[0]
	int markerOffset = isDrawnDiscrete() ? static_cast<int>(0.5 * activeWidth()*xZoom / stepCount) : 0;
	double xRng = xRange();
	if (!m_plots.empty() && m_plots[0]->data()->GetRangeType() == Discrete)
		xRng += 1;

	double visibleRng = xRng / xZoom;
	double startXVal = clamp(xBounds()[0], xBounds()[1], xBounds()[0] + ( ((static_cast<double>(-translationX)) / (activeWidth()*xZoom)) * xRng) );
	double endXVal = clamp(xBounds()[0], xBounds()[1], startXVal + visibleRng);
	if (!categoricalAxis())
	{
		// check for overlap:
		bool overlap;
		do
		{
			stepWidth = xRng / stepCount;
			overlap = false;
			for (size_t i = 0; i<stepCount && !overlap; ++i)
			{
				double value = xBounds()[0] + static_cast<double>(i) * stepWidth;
				if (value < startXVal || value > endXVal)
					continue;
				QString text = getXAxisTickMarkLabel(value, stepWidth);
				int markerX = markerPos(i, stepCount, activeWidth()*xZoom, markerOffset);
				int textX = textPos(markerX, i, stepCount, fm.width(text));
				int next_markerX = markerPos(i + 1, stepCount, activeWidth()*xZoom, markerOffset);
				int next_textX = textPos(next_markerX, i + 1, stepCount, fm.width(text));
				int textWidth = fm.width(text) + fm.width("M");
				overlap = (textX + textWidth) >= next_textX;
			}
			if (overlap)
				stepCount /= 2;
		} while (overlap && stepCount > 1);
	}
	else
	{
		QFont font = painter.font();
		font.setPointSize(CATEGORICAL_FONT_SIZE);
		painter.setFont(font);
		fm = painter.fontMetrics();
		stepWidth = xRng / stepCount;
	}

	stepCount = std::max(static_cast<size_t>(1), stepCount); // at least one step
	for (int i = 0; i <= stepCount; ++i)
	{
		double value = xBounds()[0] + static_cast<double>(i) * stepWidth;
		if (value < startXVal || value > endXVal)
			continue;
		QString text = getXAxisTickMarkLabel(value, stepWidth);
		if (isDrawnDiscrete() && i == stepCount && text.length() < 3)
			break;	// avoid last tick for discrete ranges

		int markerX = markerPos(i, stepCount, activeWidth()*xZoom, markerOffset);
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
	int stepNumber = std::min(Y_AXIS_STEPS, static_cast<int>(aheight / (m_fontHeight*1.1)));
	stepNumber = std::max(1, stepNumber);	// make sure there's at least 2 steps
	const double step = 1.0 / (stepNumber * yZoom);
	double logMax = LogFunc(static_cast<double>(m_yBounds[1]));

	for (int i = 0; i <= stepNumber; ++i)
	{
		double pos = step * i;
		double yValue = (m_yMappingMode == Linear) ? pos * m_yBounds[1] :
			/* Log: */ std::pow(LogBase, logMax / yZoom - (Y_AXIS_STEPS - i));
		QString text = DblToStringWithUnits(yValue);
		int y = -static_cast<int>(pos * aheight * yZoom) - 1;
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
	for (auto it = m_plots.constBegin(); it != m_plots.constEnd(); ++it)
		(*it)->update();
}

void iAChartWidget::setYBounds(iAPlotData::DataType valMin, iAPlotData::DataType valMax)
{
	m_customYBounds = true;
	m_yBounds[0] = valMin;
	m_yBounds[1] = valMax;
	for (auto it = m_plots.constBegin(); it != m_plots.constEnd(); ++it)
		(*it)->update();
}

void iAChartWidget::resetYBounds()
{
	m_customYBounds = false;
	m_yBounds[0] = 0;
	m_yBounds[1] = getMaxYDataValue();
}

void iAChartWidget::updateYBounds()
{
	if (m_customYBounds)
		return;
	m_yBounds[0] = 0;
	if (m_plots.empty())
		m_yBounds[1] = 1;
	else
		m_yBounds[1] = getMaxYDataValue();
}

void iAChartWidget::updateXBounds()
{
	if (m_customXBounds)
		return;
	if (m_plots.empty())
	{
		m_xBounds[0] = 0;
		m_xBounds[1] = 1;
		m_maxXAxisSteps = MAX_X_AXIS_STEPS;
	}
	else
	{
		m_xBounds[0] = std::numeric_limits<double>::max();
		m_xBounds[1] = std::numeric_limits<double>::lowest();
		m_maxXAxisSteps = 0;
		for (auto plot : m_plots)
		{
			m_xBounds[0] = std::min(m_xBounds[0], plot->data()->XBounds()[0]);
			m_xBounds[1] = std::max(m_xBounds[1], plot->data()->XBounds()[1]);
			m_maxXAxisSteps = std::max(m_maxXAxisSteps, plot->GetData()->GetNumBin());
		}
	}
}

void iAChartWidget::updateBounds()
{
	updateXBounds();
	updateYBounds();
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

int iAChartWidget::diagram2PaintX(double x) const
{
	if (m_plots.empty())
		return x;
	double screenX = (x - xBounds()[0]) * activeWidth() * xZoom / xRange();
	screenX = clamp(0.0, activeWidth()*xZoom, screenX);
	return static_cast<int>(round(screenX));
}

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
		return MAX_X_ZOOM;
	double numBin = m_plots[0]->data()->GetNumBin();
	return std::max(std::min(MAX_X_ZOOM, numBin), 1.0);
}

void iAChartWidget::setYMappingMode(AxisMappingType drawMode)
{
	if (m_yMappingMode == drawMode)
		return;
	m_yMappingMode = drawMode;
	createYConverter();
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
	updateBounds();
}

void iAChartWidget::removePlot(QSharedPointer<iAPlot> plot)
{
	if (!plot)
		return;
	int idx = m_plots.indexOf(plot);
	if (idx != -1)
		m_plots.remove(idx);
	updateBounds();
}

QVector<QSharedPointer<iAPlot> > const & iAChartWidget::plots()
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

void iAChartWidget::drawPlots(QPainter &painter, size_t startBin, size_t endBin)
{
	if (m_plots.empty())
	{
		painter.scale(1, -1);
		painter.drawText(QRect(-translationX, -translationY, activeWidth(), -activeHeight()), Qt::AlignCenter, "Chart not (yet) available.");
		painter.scale(1, -1);
		return;
	}
	// TODO: does not consider differing minimum x values!
	double binWidth = activeWidth() * xZoom / m_maxXAxisSteps;
	for (auto it = m_plots.constBegin(); it != m_plots.constEnd(); ++it)
		if ((*it)->visible())
			(*it)->draw(painter, binWidth, startBin, std::min(endBin, (*it)->GetData()->GetNumBin()), m_yMapper);
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
				zoomAlongY(-pow(Y_ZOOM_STEP,-diff)+yZoomStart, false);
			else
				zoomAlongY(pow(Y_ZOOM_STEP,diff)+yZoomStart, false);
			update();
		}
		break;
	}
	showDataTooltip(event);
}

void iAChartWidget::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	if (!m_yMapper)
		createYConverter();
	m_yMapper->update(m_yMappingMode == Linear || m_yBounds > 0 ? m_yBounds[0] : 1, m_yBounds[1], 0, yZoom*activeHeight());
	QFontMetrics fm = painter.fontMetrics();
	m_fontHeight = fm.height();
	m_yMaxTickLabelWidth = fm.width("4.44M");
	painter.setRenderHint(QPainter::Antialiasing);
	drawBackground(painter);
	size_t startBin = 0, endBin = 0;
	if (m_plots.size() > 0)
	{
		size_t numBin = m_maxXAxisSteps;
		double visibleBins = numBin / xZoom;
		double startBinDbl = clamp(0.0, static_cast<double>(numBin), (static_cast<double>(abs(translationX)) / (activeWidth()*xZoom)) * numBin);
		// for range to be correctly considered, above needs to be calculated in double, and +1 below to make sure a last partial bin is also drawn
		endBin = clamp(static_cast<size_t>(0), numBin, static_cast<size_t>(startBinDbl + visibleBins + 1) );
		startBin = static_cast<size_t>(startBinDbl);
	}

	painter.translate(translationX + leftMargin(), -bottomMargin());
	drawImageOverlays(painter);
	//change the origin of the window to left bottom
	painter.translate(0, geometry().height());
	painter.scale(1, -1);

	drawPlots(painter, startBin, endBin);
	for (double x : m_xMarker.keys())
	{
		QColor color = m_xMarker[x];
		painter.setPen(color);
		QLine line;
		QRect diagram = geometry();		// TODO: does not work with mixed range types!
		double pos = diagram2PaintX(x) + (isDrawnDiscrete() ? activeWidth() * xZoom / m_maxXAxisSteps / 2 : 0);
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
