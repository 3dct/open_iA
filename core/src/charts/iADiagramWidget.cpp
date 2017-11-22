/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "iADiagramWidget.h"
#include "iAMathUtility.h"
#include "iAPlot.h"
#include "iAStringHelper.h"

#include <QAction>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QPainter>
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
	const int LEFT_MARGIN = 60;
	const int BOTTOM_MARGIN = 40;
	const int TEXT_X = 15;
	const int Y_AXIS_STEPS = 5;
	const size_t MAX_X_AXIS_STEPS = 32 * static_cast<size_t>(MAX_X_ZOOM);

	class LinearConverter : public CoordinateConverter
	{
	public:
		LinearConverter(double yZoom, double yMin, double yMax, int height)
		{
			LinearConverter::update(yZoom, yMax, yMin, height);
		}
		double Diagram2ScreenY(double y) const override
		{
			return (y-yMin) * yScaleFactor;
		}
		double Screen2DiagramY(double y) const override
		{
			return y / yScaleFactor + yMin;
		}
		bool equals(QSharedPointer<CoordinateConverter> other) const override
		{
			LinearConverter* linearOther = dynamic_cast<LinearConverter*>(other.data());
			return (linearOther != 0 && yScaleFactor == linearOther->yScaleFactor);
		}
		QSharedPointer<CoordinateConverter> clone() override
		{
			return QSharedPointer<CoordinateConverter>(new LinearConverter(*this));
		}
		void update(double yZoom, double yMax, double yMinValueBiggerThanZero, int height) override
		{
			yMin = yMinValueBiggerThanZero;
			if (yMax)
				yScaleFactor = (double)(height - 1) / (yMax-yMin) *yZoom;
			else
				yScaleFactor = 1;
		}
	private:
		LinearConverter(LinearConverter const & other) :
			yScaleFactor(other.yScaleFactor)
		{
		}
		double yScaleFactor;
		double yMin;
	};

	class LogarithmicConverter : public CoordinateConverter
	{
	public:
		LogarithmicConverter(double yZoom, double yMax, double yMinValueBiggerThanZero, int height)
		{
			LogarithmicConverter::update(yZoom, yMax, yMinValueBiggerThanZero, height);
		}

		double Diagram2ScreenY(double y) const override
		{
			if (y <= 0)
				return 0;

			double yLog = LogFunc(y);

			yLog = clamp(yMinLog, yMaxLog, yLog);

			return mapValue(
				yMinLog, yMaxLog,
				0.0, static_cast<double>(height * yZoom),
				yLog
			);
		}
		double Screen2DiagramY(double y) const override
		{
			double yLog = mapValue(
				0.0, static_cast<double>(height * yZoom),
				yMinLog, yMaxLog,
				y
			);
			return std::pow(LogBase, yLog);
		}
		bool equals(QSharedPointer<CoordinateConverter> other) const  override
		{
			LogarithmicConverter* logOther = dynamic_cast<LogarithmicConverter*>(other.data());
			return (logOther && yZoom == logOther->yZoom &&
				yMaxLog == logOther->yMaxLog && yMinLog == logOther->yMinLog &&
				height == logOther->height);
		}
		QSharedPointer<CoordinateConverter> clone() override
		{
			return QSharedPointer<CoordinateConverter>(new LogarithmicConverter(*this));
		}
		void update(double yZoom, double yMax, double yMinValueBiggerThanZero, int height) override
		{
			this->yZoom = yZoom;
			yMaxLog = LogFunc(yMax);
			yMinLog = LogFunc(yMinValueBiggerThanZero) - 1;
			this->height = height;
		}
	private:
		LogarithmicConverter(LogarithmicConverter const & other) :
			yZoom(other.yZoom),
			yMaxLog(other.yMaxLog),
			yMinLog(other.yMinLog),
			height(other.height)
		{}
		double yZoom;
		double yMaxLog, yMinLog;
		int height;
	};
}

iADiagramWidget::iADiagramWidget(QWidget* parent, QString const & xLabel, QString const & yLabel):
	QGLWidget(parent),
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
	leftMargin(LEFT_MARGIN),
	mode(NO_MODE),
	m_contextMenuVisible(false),
	m_customXBounds(false),
	m_customYBounds(false),
	m_yMappingMode(Linear),
	m_contextMenu(new QMenu(this)),
	m_showTooltip(true),
	m_showXAxisLabel(true),
	m_captionPosition(Qt::AlignCenter | Qt::AlignBottom),
	m_draw(true)
{
	UpdateBounds();
	setMouseTracking(true);
	setFocusPolicy(Qt::WheelFocus);
	setNewSize();
	m_drawBuffer = QImage(width, height, QImage::Format_ARGB32);
	setAutoFillBackground(false);
}

iADiagramWidget::~iADiagramWidget()
{
	delete m_contextMenu;
}

void iADiagramWidget::wheelEvent(QWheelEvent *event)
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
	redraw();
	event->accept();
}

void iADiagramWidget::resizeEvent(QResizeEvent *event)
{
	if ( (this->geometry().width() != width) || (this->geometry().height() != height) )
	{
		setNewSize();
		//create a QImage newImage with the new window size and load to the QImage image
		QImage newImage(width, height, QImage::Format_ARGB32);
		m_drawBuffer = newImage;
		m_draw = true;
		repaint();
	}
	QWidget::resizeEvent(event);
}

void iADiagramWidget::leaveEvent(QEvent*)
{
	this->clearFocus();
}

void iADiagramWidget::setNewSize()
{
	width = this->geometry().width();
	height = this->geometry().height();
}

void iADiagramWidget::zoomAlongY(double value, bool deltaMode)
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
	yZoom = clamp(MIN_Y_ZOOM, MAX_Y_ZOOM, yZoom);;
}

void iADiagramWidget::zoomAlongX(double value, int x, bool deltaMode)
{
	int xZoomBefore = xZoom;
	int translationXBefore = translationX;
	// don't do anything if we're already at the limit
	if ( (deltaMode &&  (value < 0    && xZoom == 1.0) || (value > 0           && xZoom == MaxXZoom())) ||
		 (!deltaMode && (value <= 1.0 && xZoom == 1.0) || (value >= MaxXZoom() && xZoom == MaxXZoom())) )
	{
		return;
	}
	int absoluteX = x-translationX-LeftMargin();
	double absoluteXRatio = (double)absoluteX/((ActiveWidth()-1)*xZoom);
	if (deltaMode)
		if (value /* = delta */ > 0)
			xZoom *= X_ZOOM_STEP;
		else
			xZoom /= X_ZOOM_STEP;
	else
		xZoom = value;

	xZoom = clamp(MIN_X_ZOOM, MaxXZoom(), xZoom);

	int absXAfterZoom = (int)(ActiveWidth()*xZoom*absoluteXRatio);

	translationX = clamp(-static_cast<int>(ActiveWidth() * (xZoom-1)), 0,
		-absXAfterZoom +x -LeftMargin());

	if (xZoomBefore != xZoom || translationXBefore != translationX)
		emit XAxisChanged();
}

int iADiagramWidget::ActiveWidth() const
{
	return width - LeftMargin();
}

int iADiagramWidget::ActiveHeight() const
{
	return height - BottomMargin();
}

int iADiagramWidget::Height() const
{
	return height;
}

void iADiagramWidget::SetXCaption(QString const & caption)
{
	xCaption = caption;
}

double const * iADiagramWidget::XBounds() const
{
	return m_xBounds;
}

double iADiagramWidget::XRange() const
{
	return m_xBounds[1] - m_xBounds[0];
}


int iADiagramWidget::BottomMargin() const
{
	if (!m_showXAxisLabel)
	{
		return BOTTOM_MARGIN / 2.0 /* TODO: estimation for font height only. use real values! */;
	}
	return BOTTOM_MARGIN;
}

iAPlotData::DataType iADiagramWidget::GetMaxYDataValue() const
{
	iAPlotData::DataType maxVal = std::numeric_limits<iAPlotData::DataType>::lowest();
	for (auto plot : m_plots)
		maxVal = std::max(plot->GetData()->YBounds()[1], maxVal);
	return maxVal;
}

iAPlotData::DataType const * iADiagramWidget::YBounds() const
{
	return m_yBounds;
}

QSharedPointer<CoordinateConverter> const iADiagramWidget::YMapper() const
{
	return m_yConverter;
}


void iADiagramWidget::CreateYConverter()
{
	if (m_yMappingMode == Linear)
		m_yConverter = QSharedPointer<CoordinateConverter>(new LinearConverter(yZoom, m_yBounds[0], m_yBounds[1], ActiveHeight() - 1));
	else
		// 1 - smallest value larger than 0. TODO: find that from data!
		m_yConverter = QSharedPointer<CoordinateConverter>(new LogarithmicConverter(yZoom, m_yBounds[1], 1, ActiveHeight() - 1));
}


void iADiagramWidget::DrawEverything()
{
	if (!m_yConverter)
		CreateYConverter();
	m_yConverter->update(yZoom, m_yBounds[1], m_yMappingMode == Linear? m_yBounds[0] : 1, ActiveHeight());
	QPainter painter(&m_drawBuffer);
	painter.setRenderHint(QPainter::Antialiasing);
	DrawBackground(painter);
	painter.translate(translationX + LeftMargin(), -BottomMargin());
	DrawImageOverlays(painter);
	//change the origin of the window to left bottom
	painter.translate(0, height);
	painter.scale(1, -1);

	DrawPlots(painter);
	DrawAfterPlots(painter);

	painter.scale(1, -1);
	painter.setRenderHint(QPainter::Antialiasing, false);
	DrawAxes(painter);

	m_draw = false;
}

void iADiagramWidget::DrawImageOverlays(QPainter& painter)
{
	QRect targetRect = geometry();
	int yTranslate = -(yZoom - 1) * (targetRect.height());
	targetRect.setHeight(targetRect.height() - targetRect.top() - 1);
	targetRect.setWidth((targetRect.width() - LeftMargin()) * xZoom);
	targetRect.setTop(targetRect.top() + yTranslate);
	targetRect.setLeft(0);
	for (int i = 0; i < m_overlays.size(); ++i)
	{
		painter.drawImage(targetRect, *(m_overlays[i].data()), m_overlays[i]->rect());
	}
}

void iADiagramWidget::DrawAfterPlots(QPainter& painter)
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
		return number * M_PI / 180;
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

QString iADiagramWidget::GetXAxisTickMarkLabel(double value, int placesBeforeComma, int requiredPlacesAfterComma)
{
	if ((!m_plots.empty() && m_plots[0]->GetData()->GetRangeType() == Continuous) || requiredPlacesAfterComma > 1)
	{
		QString result = QString::number(value, 'g', ((value > 0) ? placesBeforeComma + requiredPlacesAfterComma : requiredPlacesAfterComma));
		if (result.contains("e")) // only 2 digits for scientific notation:
			result = QString::number(value, 'g', 2);
		return result;
	}
	else
		return QString::number(static_cast<long long>(value), 10);
}

void iADiagramWidget::DrawAxes(QPainter& painter)
{
	DrawXAxis(painter);
	DrawYAxis(painter);
}

size_t iADiagramWidget::MaxXAxisSteps() const
{
	if (!m_plots.empty())
		return m_plots[0]->GetData()->GetNumBin();
	else
		return MAX_X_AXIS_STEPS;
}

bool iADiagramWidget::CategoricalAxis() const
{
	if (!m_plots.empty())
		return (m_plots[0]->GetData()->GetRangeType() == Categorical);
	else
		return false;
}

void iADiagramWidget::DrawXAxis(QPainter &painter)
{
	painter.setPen(QWidget::palette().color(QPalette::Text));

	const int MINIMUM_MARGIN = 8;
	const int TextAxisDistance = 2;
	QFontMetrics fm = painter.fontMetrics();
	size_t stepCount = MaxXAxisSteps();
	double stepWidth;
	// for discrete x axis variables, marker&caption should be in middle of value; MaxXAxisSteps() = numBin of plot[0]
	int markerOffset = IsDrawnDiscrete() ? static_cast<int>(0.5 * width / stepCount) : 0;
	double xRange = XRange();
	if (!m_plots.empty() && m_plots[0]->GetData()->GetRangeType() == Discrete)
		xRange += 1;
	if (!CategoricalAxis())
	{
		// check for overlap:
		bool overlap;
		do
		{
			stepWidth = xRange / stepCount;
			overlap = false;
			for (size_t i = 0; i<stepCount && !overlap; ++i)
			{
				double value = XBounds()[0] + static_cast<double>(i) * stepWidth;
				int placesBeforeComma = requiredDigits(value);
				int placesAfterComma = (stepWidth < 10) ? requiredDigits(10 / stepWidth) : 0;
				QString text = GetXAxisTickMarkLabel(value, placesBeforeComma, placesAfterComma);
				int markerX = markerPos(i, stepCount, ActiveWidth()*xZoom, markerOffset);
				int textX = textPos(markerX, i, stepCount, fm.width(text));
				int next_markerX = markerPos(i + 1, stepCount, ActiveWidth()*xZoom, markerOffset);
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
	}

	stepCount = std::max(static_cast<size_t>(1), stepCount); // at least one step
	for (int i = 0; i <= stepCount; ++i)
	{
		double value = XBounds()[0] + static_cast<double>(i) * stepWidth;
		int placesBeforeComma = requiredDigits(value);
		int placesAfterComma = (stepWidth < 10) ? requiredDigits(10 / stepWidth) : 0;
		QString text = GetXAxisTickMarkLabel(value, placesBeforeComma, placesAfterComma);
		if (IsDrawnDiscrete() && i == stepCount && text.length() < 3)
			break;	// avoid last tick for discrete ranges

		int markerX = markerPos(i, stepCount, ActiveWidth()*xZoom, markerOffset);
		painter.drawLine(markerX, (int)(BottomMargin()*0.1), markerX, -1);
		int textX = textPos(markerX, i, stepCount, fm.width(text));
		int textY = fm.height() + TextAxisDistance;
		painter.translate(textX, textY);
		painter.drawText(0, 0, text);
		painter.translate(-textX, -textY);
	}

	//draw the x axis
	painter.setPen(QWidget::palette().color(QPalette::Text));
	painter.drawLine(0, -1, (int)((ActiveWidth())*xZoom), -1);

	if (m_showXAxisLabel)
	{
		//write the x axis label
		QPointF textPos(
			m_captionPosition.testFlag(Qt::AlignCenter) ? (int)(ActiveWidth() * 0.45 - translationX) : 0 /* left-aligned */,
			m_captionPosition.testFlag(Qt::AlignBottom) ? BottomMargin() - fm.descent() - 1 : -Height() + BottomMargin() + fm.height()
		);
		painter.drawText(textPos, xCaption);
	}
}

void iADiagramWidget::DrawYAxis(QPainter &painter)
{
	if (LeftMargin() <= 0)
	{
		return;
	}
	painter.save();
	painter.translate(-translationX, 0);
	QFontMetrics fm = painter.fontMetrics();
	int fontHeight = fm.height();

	int activeHeight = ActiveHeight() - 1;
	painter.fillRect(QRect(0, BottomMargin(), -LeftMargin(), -(activeHeight + BottomMargin() + 1)),
		QBrush(QWidget::palette().color(QWidget::backgroundRole())));
	painter.setPen(QWidget::palette().color(QPalette::Text));

	// at most, make Y_AXIS_STEPS, but reduce to number actually fitting in current height:
	int stepNumber = std::min(Y_AXIS_STEPS, static_cast<int>(activeHeight / (fontHeight*1.1)));
	stepNumber = std::max(1, stepNumber);	// make sure there's at least 2 steps
	const double step = 1.0 / (stepNumber * yZoom);
	double logMax = LogFunc(static_cast<double>(m_yBounds[1]));

	for (int i = 0; i <= stepNumber; ++i)
	{
		double pos = step * i;
		double yValue =
			(m_yMappingMode == Linear) ?
			pos * m_yBounds[1] :
			/* Logarithmic: */ std::pow(LogBase, logMax / yZoom - (Y_AXIS_STEPS - i));
		QString text = DblToStringWithUnits(yValue);
		//calculate the y coordinate
		int y = -(int)(pos * activeHeight * yZoom) - 1;
		//draw a small indicator line
		painter.drawLine((int)(-LeftMargin()*0.1), y, 0, y);

		if (i == stepNumber)
			painter.drawText(TEXT_X - LeftMargin(), y + 0.75*fontHeight, text); //write the text top aligned to the indicator line
		else
			painter.drawText(TEXT_X - LeftMargin(), y + 0.25*fontHeight, text); //write the text centered to the indicator line

	}
	painter.drawLine(0, -1, 0, -(int)(activeHeight*yZoom));
	//write the y axis label
	painter.save();
	painter.rotate(-90);
	QPointF textPos(
		activeHeight*0.5 - 0.5*fm.width(yCaption),
		-LeftMargin() + fontHeight - 5);
	painter.drawText(textPos, yCaption);
	painter.restore();
	painter.restore();
}

void iADiagramWidget::redraw()
{
	m_draw = true;
	update();
}

void iADiagramWidget::SetXBounds(double valMin, double valMax)
{
	m_customXBounds = true;
	m_xBounds[0] = valMin;
	m_xBounds[1] = valMax;
	for (auto it = m_plots.constBegin(); it != m_plots.constEnd(); ++it)
		(*it)->update();
}

void iADiagramWidget::SetYBounds(iAPlotData::DataType valMin, iAPlotData::DataType valMax)
{
	m_customYBounds = true;
	m_yBounds[0] = valMin;
	m_yBounds[1] = valMax;
	for (auto it = m_plots.constBegin(); it != m_plots.constEnd(); ++it)
		(*it)->update();
}

void iADiagramWidget::ResetYBounds()
{
	m_customYBounds = false;
	m_yBounds[0] = 0;
	m_yBounds[1] = GetMaxYDataValue();
}

void iADiagramWidget::UpdateYBounds()
{
	if (m_customYBounds)
		return;
	m_yBounds[0] = 0;
	if (m_plots.empty())
		m_yBounds[1] = 1;
	else
		m_yBounds[1] = GetMaxYDataValue();
}

void iADiagramWidget::UpdateXBounds()
{
	if (m_customXBounds)
		return;
	if (m_plots.empty())
	{
		m_xBounds[0] = 0;
		m_xBounds[1] = 1;
	}
	else
	{
		m_xBounds[0] = std::numeric_limits<double>::max();
		m_xBounds[1] = std::numeric_limits<double>::lowest();
		for (auto plot : m_plots)
		{
			m_xBounds[0] = std::min(m_xBounds[0], plot->GetData()->XBounds()[0]);
			m_xBounds[1] = std::max(m_xBounds[1], plot->GetData()->XBounds()[1]);
		}
	}
}

void iADiagramWidget::UpdateBounds()
{
	UpdateXBounds();
	UpdateYBounds();
}

void iADiagramWidget::DrawBackground(QPainter &painter)
{
	painter.fillRect( rect(), QWidget::palette().color(QWidget::backgroundRole()));
}

void iADiagramWidget::resetView()
{
	translationX = 0;
	xZoom = 1.0;
	yZoom = 1.0;
	emit XAxisChanged();
	redraw();
}

int iADiagramWidget::diagram2PaintX(double x) const
{
	if (m_plots.empty())
		return x;
	double screenX = (x - XBounds()[0]) * ActiveWidth() * xZoom / XRange();
	screenX = clamp(0.0, ActiveWidth()*xZoom, screenX);
	return static_cast<int>(round(screenX));
}

long iADiagramWidget::screenX2DataBin(int x) const
{
	if (m_plots.empty())
		return x;
	double numBin = m_plots[0]->GetData()->GetNumBin();
	double diagX = static_cast<double>(x - translationX - LeftMargin()) * numBin / (ActiveWidth() * xZoom);
	diagX = clamp(0.0, numBin, diagX);
	return static_cast<long>(round(diagX));
}

int iADiagramWidget::dataBin2ScreenX(long x) const
{
	if (m_plots.empty())
		return x;
	double numBin = m_plots[0]->GetData()->GetNumBin();
	double screenX = static_cast<double>(x) * ActiveWidth() * xZoom / (numBin);
	screenX = clamp(0.0, ActiveWidth()*xZoom, screenX);
	return static_cast<int>(round(screenX));
}

bool iADiagramWidget::IsContextMenuVisible() const
{
	return m_contextMenuVisible;
}

QPoint iADiagramWidget::ContextMenuPos() const
{
	return m_contextPos;
}

double iADiagramWidget::MaxXZoom() const
{
	if (m_plots.empty())
		return MAX_X_ZOOM;
	double numBin = m_plots[0]->GetData()->GetNumBin();
	return std::max(std::min(MAX_X_ZOOM, numBin), 1.0);
}

void iADiagramWidget::SetYMappingMode(AxisMappingType drawMode)
{
	if (m_yMappingMode == drawMode)
		return;
	m_yMappingMode = drawMode;
	CreateYConverter();
}

void iADiagramWidget::SetCaptionPosition(QFlags<Qt::AlignmentFlag> captionPosition)
{
	m_captionPosition = captionPosition;
}

void iADiagramWidget::SetShowXAxisLabel(bool show)
{
	m_showXAxisLabel = show;
}

void iADiagramWidget::AddPlot(QSharedPointer<iAPlot> plot)
{
	assert(plot);
	if (!plot)
		return;
	m_plots.push_back(plot);
	UpdateBounds();
}

void iADiagramWidget::RemovePlot(QSharedPointer<iAPlot> plot)
{
	if (!plot)
		return;
	int idx = m_plots.indexOf(plot);
	if (idx != -1)
		m_plots.remove(idx);
	UpdateBounds();
}

QVector<QSharedPointer<iAPlot> > const & iADiagramWidget::Plots()
{
	return m_plots;
}


bool iADiagramWidget::IsDrawnDiscrete() const
{
	for (auto plot : m_plots)
		if (!((plot->GetData()->GetRangeType() == Discrete && (XRange() <= plot->GetData()->GetNumBin()))
			  || plot->GetData()->GetRangeType() == Categorical))
			return false;
	return !m_plots.empty();
}

void iADiagramWidget::AddImageOverlay(QSharedPointer<QImage> imgOverlay)
{
	m_overlays.push_back(imgOverlay);
}

void iADiagramWidget::RemoveImageOverlay(QImage * imgOverlay)
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

void iADiagramWidget::DrawPlots(QPainter &painter)
{
	if (m_plots.empty())
	{
		painter.scale(1, -1);
		painter.drawText(QRect(0, 0, ActiveWidth(), -ActiveHeight()), Qt::AlignCenter, "Chart not (yet) available.");
		painter.scale(1, -1);
		return;
	}
	double binWidth = ActiveWidth() * xZoom / m_plots[0]->GetData()->GetNumBin();
	for (auto it = m_plots.constBegin(); it != m_plots.constEnd(); ++it)
		if ((*it)->Visible())
			(*it)->draw(painter, binWidth, m_yConverter);
}

void iADiagramWidget::showDataTooltip(QMouseEvent *event)
{
	if (m_plots.empty() || !m_showTooltip)
		return;
	int xPos = clamp(0, width - 1, event->x());
	size_t numBin = m_plots[0]->GetData()->GetNumBin();
	assert(numBin > 0);
	int nthBin = static_cast<int>((((xPos - translationX - LeftMargin()) * numBin) / (ActiveWidth())) / xZoom);
	nthBin = clamp(0, static_cast<int>(numBin), nthBin);
	if (xPos == width - 1)
		nthBin = static_cast<int>(numBin) - 1;
	QString toolTip;
	if (yCaption.isEmpty())
		toolTip = QString("%1: ").arg(m_plots[0]->GetData()->GetBinStart(nthBin));
	else
		toolTip = QString("%1: %2\n%3:").arg(xCaption).arg(m_plots[0]->GetData()->GetBinStart(nthBin)).arg(yCaption);
	for (auto plot : m_plots)
	{
		auto data = plot->GetData();
		if (!data || !data->GetRawData())
			continue;
		toolTip += QString::number(data->GetRawData()[nthBin]);
	}
	QToolTip::showText(event->globalPos(), toolTip, this);
}

void iADiagramWidget::changeMode(int newMode, QMouseEvent *event)
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

void iADiagramWidget::mouseReleaseEvent(QMouseEvent *event)  
{
	if (event->button() == Qt::LeftButton)
		redraw();
	this->mode = NO_MODE;
}

void iADiagramWidget::mousePressEvent(QMouseEvent *event)  
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
		redraw();
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

void iADiagramWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch(mode)
	{
	case NO_MODE: /* do nothing */ break;
	case MOVE_VIEW_MODE:
		translationX = clamp(-static_cast<int>(ActiveWidth() * (xZoom-1)), 0,
			translationStartX + event->x() - dragStartPosX);
		emit XAxisChanged();
		translationY = translationStartY + event->y() - dragStartPosY;
		translationY = clamp(static_cast<int>(-(height * yZoom - height)), static_cast<int>(height * yZoom - height), translationY);
		redraw();
		break;
	case X_ZOOM_MODE:
		zoomAlongX(((zoomY-event->y())/2.0)+xZoomStart, zoomX, false);
		redraw();
		break;
	case Y_ZOOM_MODE:
		{
			int diff = (zoomY-event->y())/2.0;
			if (diff < 0)
				zoomAlongY(-pow(Y_ZOOM_STEP,-diff)+yZoomStart, false);
			else
				zoomAlongY(pow(Y_ZOOM_STEP,diff)+yZoomStart, false);
			redraw();
		}
		break;
	}
	showDataTooltip(event);
}

void iADiagramWidget::paintEvent(QPaintEvent * e)
{
	if (m_draw)
		DrawEverything();
	QPainter painter(this);
	painter.drawImage(QRectF(0, 0, geometry().width(), geometry().height()), m_drawBuffer);
}

void iADiagramWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Alt ||
		event->key() == Qt::Key_AltGr ||
		event->key() == Qt::Key_Escape)
		m_contextMenuVisible = false;
}

void iADiagramWidget::AddContextMenuEntries(QMenu* contextMenu)
{}

void iADiagramWidget::contextMenuEvent(QContextMenuEvent *event)
{
	m_contextPos = event->pos();
	m_contextMenu->clear();
	m_contextMenu->addAction(QIcon(":/images/resetView.png"), tr("Reset view"), this, SLOT(resetView()));
	QAction *showTooltipAction = new QAction(tr("Show tooltip"), this);
	showTooltipAction->setCheckable(true);
	showTooltipAction->setChecked(m_showTooltip);
	connect(showTooltipAction, SIGNAL(toggled(bool)), this, SLOT(showTooltip(bool)));
	m_contextMenu->addAction(showTooltipAction);
	m_contextMenu->addAction(QIcon(":/images/save.png"), tr("Export data"), this, SLOT(ExportData()));
	m_contextMenu->addSeparator();
	AddContextMenuEntries(m_contextMenu);
	m_contextMenuVisible = true;
	m_contextMenu->exec(event->globalPos());
}


void iADiagramWidget::ExportData()
{
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
	for (int b = 0; b < m_plots[0]->GetData()->GetNumBin(); ++b)
	{
		out << m_plots[0]->GetData()->GetBinStart(b);
		for (int p = 0; p < m_plots.size(); ++p)
		{
			out << "," << m_plots[p]->GetData()->GetRawData()[b];
		}
		out << std::endl;
	}
	out.close();
}

void iADiagramWidget::showTooltip(bool toggled)
{
	m_showTooltip = toggled;
}
