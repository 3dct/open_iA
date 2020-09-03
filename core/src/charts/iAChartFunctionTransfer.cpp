/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAChartFunctionTransfer.h"

#include "charts/iAChartWithFunctionsWidget.h"
#include "iAConsole.h"
#include "iAMapper.h"
#include "iAMathUtility.h"
#include "mdichild.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QColorDialog>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

iAChartTransferFunction::iAChartTransferFunction(iAChartWithFunctionsWidget *chart, QColor color):
	iAChartFunction(chart),
	m_rangeSliderHandles(false),
	m_selectedPoint(-1),
	m_color(color),
	m_opacityTF(nullptr),
	m_colorTF(nullptr)
{
	m_gradient.setSpread(QGradient::PadSpread);
}

void iAChartTransferFunction::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (m_chart->selectedFunction() == this);

	QPen pen = painter.pen();
	pen.setColor(color); pen.setWidth(lineWidth);
	QPen pen1 = painter.pen();
	pen1.setColor(color); pen1.setWidth(1);
	QPen redPen = painter.pen();
	redPen.setColor(Qt::red); redPen.setWidth(1);

	painter.setPen(pen);
	painter.setBrush(QColor(128, 128, 128, 255));

	double gradientWidth = m_chart->chartWidth()*m_chart->xZoom();

	m_gradient = QLinearGradient();
	m_gradient.setStart(0, 0);
	m_gradient.setFinalStop(gradientWidth, 0);

	// draw opacity and color tf
	if (m_opacityTF->GetSize() != m_colorTF->GetSize())
	{
		DEBUG_LOG(QString("Definition mismatch: opacity TF has %1 positions, color TF %2!")
			.arg(m_opacityTF->GetSize())
			.arg(m_colorTF->GetSize()));
		return;
	}
	double opacityTFValue[4];
	double colorTFValue[6];

	m_opacityTF->GetNodeValue(0, opacityTFValue);
	m_colorTF->GetNodeValue(0, colorTFValue);

	int x1 = m_chart->xMapper().srcToDst(opacityTFValue[0]);
	int y1 = opacity2PixelY(opacityTFValue[1]);

	QColor c; c.setRgbF(colorTFValue[1], colorTFValue[2], colorTFValue[3], 0.588);
	m_gradient.setColorAt(static_cast<double>(x1) / gradientWidth, c );

	int lastX = x1;
	for ( int i = 1; i < m_opacityTF->GetSize(); i++)
	{
		m_opacityTF->GetNodeValue(i, opacityTFValue);
		m_colorTF->GetNodeValue(i, colorTFValue);

		int x2 = m_chart->xMapper().srcToDst(opacityTFValue[0]);
		if (x2 == lastX)
		{
			++x2;
		}
		lastX = x2;
		int y2 = opacity2PixelY(opacityTFValue[1]);
		painter.drawLine(x1, y1, x2, y2); // draw line
		if (active)
		{
			if (!m_rangeSliderHandles)
			{
				if (i - 1 == m_selectedPoint)
				{
					painter.setPen(redPen);
					painter.setBrush(QBrush(c));
					painter.drawEllipse(x1 - iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS, y1 - iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS,
						iAChartWithFunctionsWidget::SELECTED_POINT_SIZE, iAChartWithFunctionsWidget::SELECTED_POINT_SIZE);
				}
				else
				{
					painter.setPen(pen1);
					painter.setBrush(QBrush(c));
					painter.drawEllipse(x1 - iAChartWithFunctionsWidget::POINT_RADIUS, y1 - iAChartWithFunctionsWidget::POINT_RADIUS,
						iAChartWithFunctionsWidget::POINT_SIZE, iAChartWithFunctionsWidget::POINT_SIZE);
				}
			}
			else
			{
				if ( i - 1 == m_selectedPoint &&  i - 1 > 0 )
				{
					painter.setPen( redPen );
					painter.setBrush( QBrush( QColor( 254, 153, 41, 150 ) ) );
					QRectF rectangle( x1 - SELECTED_PIE_RADIUS, y1 - SELECTED_PIE_RADIUS,
										SELECTED_PIE_SIZE, SELECTED_PIE_SIZE );
					painter.drawPie( rectangle, 60 * 16, 60 * 16 );
				}
				else if ( i - 1 > 0 )
				{
					painter.setPen( redPen );
					painter.setBrush( QBrush( QColor( 254, 153, 41, 150 ) ) );
					QRectF rectangle( x1 - PIE_RADIUS, y1 - PIE_RADIUS,
										PIE_SIZE, PIE_SIZE );
					painter.drawPie( rectangle, 60 * 16, 60 * 16 );
				}
			}
		}

		painter.setPen(pen);
		c.setRgbF(colorTFValue[1], colorTFValue[2], colorTFValue[3], 0.588);
		m_gradient.setColorAt(static_cast<double>(x2) / gradientWidth, c);
		x1 = x2;
		y1 = y2;
	}
	if ( active && !m_rangeSliderHandles )
	{
		if (m_selectedPoint == m_opacityTF->GetSize()-1)
		{
			painter.setPen(redPen);
			painter.setBrush(QBrush(c));
			painter.drawEllipse(x1-iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS, y1-iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS,
				iAChartWithFunctionsWidget::SELECTED_POINT_SIZE, iAChartWithFunctionsWidget::SELECTED_POINT_SIZE);
		}
		else
		{
			painter.setPen(pen1);
			painter.setBrush(QBrush(c));
			painter.drawEllipse(x1-iAChartWithFunctionsWidget::POINT_RADIUS, y1-iAChartWithFunctionsWidget::POINT_RADIUS,
				iAChartWithFunctionsWidget::POINT_SIZE, iAChartWithFunctionsWidget::POINT_SIZE);
		}
	}
}

void iAChartTransferFunction::draw(QPainter &painter)
{
	draw(painter, m_color, 1);
}

void iAChartTransferFunction::drawOnTop(QPainter &painter)
{
	if (m_opacityTF->GetSize() == m_colorTF->GetSize())
	{
		double gradientWidth = m_chart->chartWidth()*m_chart->xZoom();

		painter.fillRect( 0, 1, gradientWidth, -(m_chart->bottomMargin()+1), m_gradient );
	}
}

int iAChartTransferFunction::selectPoint(QMouseEvent *event, int *x)
{
	int mouseX = event->x() - m_chart->leftMargin() - m_chart->xShift();
	int mouseY = m_chart->chartHeight() - event->y() - m_chart->yShift();
	int index = -1;

	double pointValue[4];
	for (int pointIndex = 0; pointIndex < m_opacityTF->GetSize(); pointIndex++)
	{
		m_opacityTF->GetNodeValue(pointIndex, pointValue);
		int pointX = m_chart->xMapper().srcToDst(pointValue[0]);
		int pointY = opacity2PixelY(pointValue[1]);
		int pointRadius = (pointIndex == m_selectedPoint) ? iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS : iAChartWithFunctionsWidget::POINT_RADIUS;
		if ( !m_rangeSliderHandles )
		{
			if (std::abs(mouseX - pointX) < pointRadius && std::abs(mouseY - pointY) < pointRadius)
			{
				index = pointIndex;
				break;
			}
		}
		else
		{
			if ( (mouseX >= pointX - SELECTED_PIE_RADIUS && mouseX <= pointX + SELECTED_PIE_RADIUS
				&& mouseY >= pointY - SELECTED_PIE_RADIUS && mouseY <= pointY)
				|| (mouseX >= pointX - PIE_RADIUS && mouseX <= pointX + PIE_RADIUS
				&& mouseY >= pointY - PIE_RADIUS && mouseY <= pointY) )
			{
				// FeatureAnalyzer: range slider widget; only handles can get selected (no end points)
				if (this->isEndPoint(pointIndex))
				{
					continue;
				}

				index = pointIndex;
				break;
			}
		}

		// TODO: determine what the use of the following block is.
		// from a cursory glance: it sets x to the pixel position of the event + 1,
		// if current x is equal to center pixel position of current point
		// Questions:
		//     - why does this happen in every loop, not only if current point is selected one?
		//     - what's the use of the +1 / not +1 distinction?
		//     - seems to be skipped for the actually selected point (because of break)...?
		//     - never called if selected point is first one?
		if (x != nullptr)
		{
			*x = (*x == pointX)? mouseX + 1: mouseX;
		}
	}

	m_selectedPoint = index;

	return index;
}

int iAChartTransferFunction::addPoint(int x, int y)
{
	m_selectedPoint = m_opacityTF->AddPoint(m_chart->xMapper().dstToSrc(x - m_chart->xShift()), pixelY2Opacity(y));
	return m_selectedPoint;
}

void iAChartTransferFunction::addColorPoint(int x, double red, double green, double blue)
{
	if (red < 0 || green < 0 || blue < 0)
	{
		QGradientStops stops = m_gradient.stops();
		double gradientWidth = m_chart->chartWidth() * m_chart->xZoom();
		double pos = x / gradientWidth;

		// find stops before and after pos
		int i = 0;
		QGradientStop stop = stops.at(0);
		while (stop.first < pos)
		{
			++i;
			if (i >= stops.size())
			{
				break;
			}
			stop = stops.at(i);
		}

		double firstPos;
		QColor firstColor;
		int idx = (i == 0) ? 0 : i-1;
		firstPos = stops.at(idx).first;
		firstColor = stops.at(idx).second;

		double secondPos;
		QColor secondColor;
		idx = (i == stops.size()) ? i-1 : i;
		secondPos = stops.at(idx).first;
		secondColor =  stops.at(idx).second;

		double secondWeight = (secondPos == firstPos) ? 1.0 : (pos-firstPos)/(secondPos-firstPos);
		double firstWeight = 1.0-secondWeight;

		red = (firstColor.red()*firstWeight +secondColor.red()*secondWeight)/255.0;
		green = (firstColor.green()*firstWeight +secondColor.green()*secondWeight)/255.0;
		blue = (firstColor.blue()*firstWeight +secondColor.blue()*secondWeight)/255.0;
	}

	m_colorTF->AddRGBPoint(m_chart->xMapper().dstToSrc(x - m_chart->xShift()), red, green, blue);
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::removePoint(int index)
{
	if (m_opacityTF->GetSize() < 2)
	{
		return;
	}
	double values[4];
	m_opacityTF->GetNodeValue(index, values);

	double cvalues[6];
	m_colorTF->GetNodeValue(index, cvalues);
	m_opacityTF->RemovePoint(values[0]);
	m_colorTF->RemovePoint(values[0]);
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::moveSelectedPoint(int x, int y)
{
	x = clamp(0, m_chart->chartWidth() - 1, x);
	y = clamp(0, m_chart->chartHeight() - 1, y);

	double dataX = m_chart->xMapper().dstToSrc(x - m_chart->xShift());
	if (m_selectedPoint != 0 && m_selectedPoint != m_opacityTF->GetSize()-1)
	{
		double nextOpacityTFValue[4];
		double prevOpacityTFValue[4];

		m_opacityTF->GetNodeValue(m_selectedPoint+1, nextOpacityTFValue);
		m_opacityTF->GetNodeValue(m_selectedPoint-1, prevOpacityTFValue);
		int newX = x;
		if (dataX >= nextOpacityTFValue[0])
		{
			newX = m_chart->xMapper().srcToDst(nextOpacityTFValue[0]) - 1;
		}
		else if (dataX <= prevOpacityTFValue[0])
		{
			newX = m_chart->xMapper().srcToDst(prevOpacityTFValue[0]) + 1;
		}
		setPointOpacity(m_selectedPoint, newX, y);
		double colorTFValue[6];
		m_colorTF->GetNodeValue(m_selectedPoint, colorTFValue);
		double chartX = m_chart->xMapper().dstToSrc(newX - m_chart->xShift());
		setPointColor(m_selectedPoint, chartX, colorTFValue[1], colorTFValue[2], colorTFValue[3]);
	}
	else
	{
		setPointOpacity(m_selectedPoint, y);
	}

	triggerOnChange();
}

void iAChartTransferFunction::changeColor(QMouseEvent *event)
{
	if (event != nullptr)
	{
		m_selectedPoint = selectPoint(event);
	}

	if (m_selectedPoint == -1)
	{
		return;
	}
	double colorTFValue[6];
	m_colorTF->GetNodeValue(m_selectedPoint, colorTFValue);
	QColorDialog colorDlg;
	colorDlg.setCurrentColor(QColor(colorTFValue[1]*255, colorTFValue[2]*255, colorTFValue[3]*255));
	if (colorDlg.exec() != QDialog::Accepted)
	{
		return;
	}

	QColor col = colorDlg.selectedColor();
	setPointColor(m_selectedPoint, colorTFValue[0], col.redF(), col.greenF(), col.blueF());
	m_colorTF->Modified();
	m_colorTF->Build();
	triggerOnChange();
}

bool iAChartTransferFunction::isEndPoint(int index) const
{
	return index == 0 || index == m_opacityTF->GetSize()-1;
}

bool iAChartTransferFunction::isDeletable(int index) const
{
	return !isEndPoint(index);
}

QString iAChartTransferFunction::name() const
{
	return QString("Transfer function (%1 points)").arg(m_opacityTF->GetSize());
}

void iAChartTransferFunction::reset()
{
	if (m_opacityTF && m_colorTF)
	{
		m_opacityTF->RemoveAllPoints();
		m_opacityTF->AddPoint(m_chart->xBounds()[0], 0.0 );
		m_opacityTF->AddPoint(m_chart->xBounds()[1], 1.0 );
		m_colorTF->RemoveAllPoints();
		m_colorTF->AddRGBPoint(m_chart->xBounds()[0], 0.0, 0.0, 0.0 );
		m_colorTF->AddRGBPoint(m_chart->xBounds()[1], 1.0, 1.0, 1.0 );
		m_colorTF->Build();
		triggerOnChange();
	}
}

#include "iAPlot.h"
#include "iAPlotData.h"

void iAChartTransferFunction::TranslateToNewRange(double const oldXRange[2])
{
	double newXRange[2];
	m_opacityTF->GetRange(newXRange);
	double offset = (m_chart->plots().size() > 0 && m_chart->plots()[0]->data()->valueType() == Discrete) ? 0.5 : 0;
	if (dblApproxEqual(newXRange[0] - offset, oldXRange[0]) &&
		dblApproxEqual(newXRange[1] + offset, oldXRange[1]))
	{
		return;
	}
	assert(m_opacityTF->GetSize() == m_colorTF->GetSize());
	for (int i = 0; i < m_opacityTF->GetSize(); ++i)
	{
		double opacity[4];
		m_opacityTF->GetNodeValue(i, opacity);
		double color[6];
		m_colorTF->GetNodeValue(i, color);
		assert(opacity[0] == color[0]);
		double newX = mapValue(oldXRange, newXRange, opacity[0]);
		color[0] = opacity[0] = clamp(newXRange[0], newXRange[1], newX);
		m_opacityTF->SetNodeValue(i, opacity);
		m_colorTF->SetNodeValue(i, color);
	}
	m_colorTF->Modified();
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::mouseReleaseEventAfterNewPoint(QMouseEvent *)
{
	double colorTFValue[6];
	m_colorTF->GetNodeValue(m_selectedPoint, colorTFValue);
	QColorDialog colorDlg;
	colorDlg.setCurrentColor(QColor(colorTFValue[1]*255, colorTFValue[2]*255, colorTFValue[3]*255));
	if (colorDlg.exec() == QDialog::Accepted)
	{
		QColor col = colorDlg.selectedColor();
		setPointColor(m_selectedPoint, colorTFValue[0], (double)col.red()/255.0, (double)col.green()/255.0, (double)col.blue()/255.0 );
	}
	else if (m_selectedPoint > 0 && m_selectedPoint < m_opacityTF->GetSize()-1)
	{
		removePoint(m_selectedPoint);
	}
}

void iAChartTransferFunction::setPointColor(int selectedPoint, double x, double red, double green, double blue)
{
	double colorVal[6] = { x, red, green, blue, 0.5, 0.0 };
	m_colorTF->SetNodeValue(selectedPoint, colorVal);
	m_colorTF->Modified();
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::setPointOpacity(int selectedPoint, int pixelX, int pixelY)
{
	double opacityVal[4] = { m_chart->xMapper().dstToSrc(pixelX - m_chart->xShift()), pixelY2Opacity(pixelY), 0.0, 0.0 };
	m_opacityTF->SetNodeValue(selectedPoint, opacityVal);
}

void iAChartTransferFunction::setPointOpacity(int selectedPoint, int pixelY)
{
	double opacityTFValues[4];
	m_opacityTF->GetNodeValue(selectedPoint, opacityTFValues);

	opacityTFValues[1] = pixelY2Opacity(pixelY);
	m_opacityTF->SetNodeValue(selectedPoint, opacityTFValues);
}

double iAChartTransferFunction::pixelY2Opacity(int pixelY)
{
	return mapToNorm(0, m_chart->chartHeight(), clamp(0, m_chart->chartHeight(), pixelY));
}

int iAChartTransferFunction::opacity2PixelY(double opacity)
{
	return mapNormTo(0, std::max(0, m_chart->chartHeight()), opacity);
}

void iAChartTransferFunction::triggerOnChange()
{
	emit changed();
}

size_t iAChartTransferFunction::numPoints() const
{
	return m_opacityTF->GetSize();
}

void iAChartTransferFunction::enableRangeSliderHandles( bool rangeSliderHandles )
{
	m_rangeSliderHandles = rangeSliderHandles;
}
