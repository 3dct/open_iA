/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAChartFunctionTransfer.h"

#include "charts/iADiagramFctWidget.h"
#include "iAConsole.h"
#include "iAMathUtility.h"
#include "mdichild.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QColorDialog>
#include <QDesktopWidget>
#include <QtXml/QDomNode>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

iAChartTransferFunction::iAChartTransferFunction(iADiagramFctWidget *chart, QColor color):
	iAChartFunction(chart),
	m_rangeSliderHandles(false),
	m_color(color),
	m_colorDlg(new QColorDialog(chart)),
	m_opacityTF(nullptr),
	m_colorTF(NULL),
	m_selectedPoint(-1)
{
	m_gradient.setSpread(QGradient::PadSpread);
}

iAChartTransferFunction::~iAChartTransferFunction()
{
	delete m_colorDlg;
}

void iAChartTransferFunction::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (chart->selectedFunction() == this);

	QPen pen = painter.pen();
	pen.setColor(color); pen.setWidth(lineWidth);
	QPen pen1 = painter.pen();
	pen1.setColor(color); pen1.setWidth(1);
	QPen redPen = painter.pen();
	redPen.setColor(Qt::red); redPen.setWidth(1);

	painter.setPen(pen);
	painter.setBrush(QColor(128, 128, 128, 255));

	QColor c;
	double gradientWidth = chart->activeWidth()*chart->xZoom();

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

	int x1, y1;
	x1 = d2iX(opacityTFValue[0]);
	y1 = d2iY(opacityTFValue[1]);

	c = QColor(colorTFValue[1]*255.0, colorTFValue[2]*255.0, colorTFValue[3]*255.0, 150);
	m_gradient.setColorAt((double)x1/gradientWidth, c );

	for ( int i = 1; i < m_opacityTF->GetSize(); i++)
	{
		m_opacityTF->GetNodeValue(i, opacityTFValue);
		m_colorTF->GetNodeValue(i, colorTFValue);

		int x2 = d2iX(opacityTFValue[0]);
		int y2 = d2iY(opacityTFValue[1]);
		painter.drawLine(x1, y1, x2, y2); // draw line
		if (active)
		{
			if (!m_rangeSliderHandles)
			{
				if (i - 1 == m_selectedPoint)
				{
					painter.setPen(redPen);
					painter.setBrush(QBrush(c));
					painter.drawEllipse(x1 - iADiagramFctWidget::SELECTED_POINT_RADIUS, y1 - iADiagramFctWidget::SELECTED_POINT_RADIUS,
						iADiagramFctWidget::SELECTED_POINT_SIZE, iADiagramFctWidget::SELECTED_POINT_SIZE);
				}
				else
				{
					painter.setPen(pen1);
					painter.setBrush(QBrush(c));
					painter.drawEllipse(x1 - iADiagramFctWidget::POINT_RADIUS, y1 - iADiagramFctWidget::POINT_RADIUS,
						iADiagramFctWidget::POINT_SIZE, iADiagramFctWidget::POINT_SIZE);
				}
			}
			else
			{
				if ( i - 1 == m_selectedPoint &&  i - 1 > 0 )
				{
					painter.setPen( redPen );
					painter.setBrush( QBrush( QColor( 254, 153, 41, 150 ) ) );
					QRectF rectangle( x1 - iADiagramFctWidget::SELECTED_PIE_RADIUS, y1 - iADiagramFctWidget::SELECTED_PIE_RADIUS,
										iADiagramFctWidget::SELECTED_PIE_SIZE, iADiagramFctWidget::SELECTED_PIE_SIZE );
					painter.drawPie( rectangle, 60 * 16, 60 * 16 );
				}
				else if ( i - 1 > 0 )
				{
					painter.setPen( redPen );
					painter.setBrush( QBrush( QColor( 254, 153, 41, 150 ) ) );
					QRectF rectangle( x1 - iADiagramFctWidget::PIE_RADIUS, y1 - iADiagramFctWidget::PIE_RADIUS,
										iADiagramFctWidget::PIE_SIZE, iADiagramFctWidget::PIE_SIZE );
					painter.drawPie( rectangle, 60 * 16, 60 * 16 );
				}
			}
		}

		painter.setPen(pen);
		c = QColor(colorTFValue[1]*255.0, colorTFValue[2]*255.0, colorTFValue[3]*255.0, 150);
		m_gradient.setColorAt((double)x2/gradientWidth, QColor(c.red(), c.green(), c.blue(), 150));
		x1 = x2;
		y1 = y2;
	}
	if ( active && !m_rangeSliderHandles )
	{
		if (m_selectedPoint == m_opacityTF->GetSize()-1)
		{
			painter.setPen(redPen);
			painter.setBrush(QBrush(c));
			painter.drawEllipse(x1-iADiagramFctWidget::SELECTED_POINT_RADIUS, y1-iADiagramFctWidget::SELECTED_POINT_RADIUS,
				iADiagramFctWidget::SELECTED_POINT_SIZE, iADiagramFctWidget::SELECTED_POINT_SIZE);
		}
		else
		{
			painter.setPen(pen1);
			painter.setBrush(QBrush(c));
			painter.drawEllipse(x1-iADiagramFctWidget::POINT_RADIUS, y1-iADiagramFctWidget::POINT_RADIUS,
				iADiagramFctWidget::POINT_SIZE, iADiagramFctWidget::POINT_SIZE);
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
		double gradientWidth = chart->activeWidth()*chart->xZoom();

		painter.fillRect( 0, 0, gradientWidth, -chart->bottomMargin(), m_gradient );
	}
}

int iAChartTransferFunction::selectPoint(QMouseEvent *event, int *x)
{
	int lx = event->x() - chart->leftMargin();
	int ly = chart->geometry().height() - event->y() - chart->bottomMargin() - chart->yShift();
	int index = -1;

	double pointValue[4];
	for (int pointIndex = 0; pointIndex < m_opacityTF->GetSize(); pointIndex++)
	{
		m_opacityTF->GetNodeValue(pointIndex, pointValue);
		int viewX, viewY;

		viewX = d2vX(pointValue[0]);
		viewY = d2vY(pointValue[1]);

		if ( !m_rangeSliderHandles )
		{
			if ( ( pointIndex == m_selectedPoint &&
				lx >= viewX - iADiagramFctWidget::SELECTED_POINT_RADIUS && lx <= viewX + iADiagramFctWidget::SELECTED_POINT_RADIUS &&
				ly >= viewY - iADiagramFctWidget::SELECTED_POINT_RADIUS && ly <= viewY + iADiagramFctWidget::SELECTED_POINT_RADIUS ) ||
				( lx >= viewX - iADiagramFctWidget::POINT_RADIUS && lx <= viewX + iADiagramFctWidget::POINT_RADIUS &&
				ly >= viewY - iADiagramFctWidget::POINT_RADIUS && ly <= viewY + iADiagramFctWidget::POINT_RADIUS ) )

			{
				index = pointIndex;
				break;
			}
		}
		else
		{
			if ( ( lx >= viewX - iADiagramFctWidget::SELECTED_PIE_RADIUS && lx <= viewX + iADiagramFctWidget::SELECTED_PIE_RADIUS
				&& ly >= viewY - iADiagramFctWidget::SELECTED_PIE_RADIUS && ly <= viewY )
				|| ( lx >= viewX - iADiagramFctWidget::PIE_RADIUS && lx <= viewX + iADiagramFctWidget::PIE_RADIUS
				&& ly >= viewY - iADiagramFctWidget::PIE_RADIUS && ly <= viewY ) )
			{
				// PorosityAnalyser: range slider widget; only handles can get selected (no end points)
				if ( this->isEndPoint( pointIndex ) )
					continue;

				index = pointIndex;
				break;
			}
		}

		if (x != NULL)
		{
			if (*x == viewX)
				*x = lx+1;
			else
				*x = lx;
		}
	}

	m_selectedPoint = index;

	return index;
}

int iAChartTransferFunction::addPoint(int x, int y)
{
	if (y < 0)
		y = 0;

	double d[2] = { v2dX(x), v2dY(y) };
	m_selectedPoint = m_opacityTF->AddPoint(d[0], d[1]);

	return m_selectedPoint;
}

void iAChartTransferFunction::addColorPoint(int x, double red, double green, double blue)
{
	if (red < 0 || green < 0 || blue < 0)
	{
		QGradientStops stops = m_gradient.stops();
		double gradientWidth = chart->activeWidth()*chart->xZoom();
		double pos = x /gradientWidth;

		// find stops before and after pos
		int i = 0;
		QGradientStop stop = stops.at(0);
		while (stop.first < pos)
		{
			i++;
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

	m_colorTF->AddRGBPoint(v2dX(x), red, green, blue);
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::removePoint(int index)
{
	if (m_opacityTF->GetSize()>2)
	{
		double values[4];
		m_opacityTF->GetNodeValue(index, values);

		double cvalues[6];
		m_colorTF->GetNodeValue(index, cvalues);
		m_opacityTF->RemovePoint(values[0]);
		m_colorTF->RemovePoint(values[0]);
		m_colorTF->Build();
		triggerOnChange();
	}
}

void iAChartTransferFunction::moveSelectedPoint(int x, int y)
{
	if (x > chart->activeWidth()-1) x = chart->activeWidth() - 1;
	y = clamp(0, chart->geometry().height() - chart->bottomMargin() - 1, y);

	double dataX = v2dX(x);
	if (m_selectedPoint != 0 && m_selectedPoint != m_opacityTF->GetSize()-1)
	{
		double nextOpacityTFValue[4];
		double prevOpacityTFValue[4];

		m_opacityTF->GetNodeValue(m_selectedPoint+1, nextOpacityTFValue);
		m_opacityTF->GetNodeValue(m_selectedPoint-1, prevOpacityTFValue);

		if (dataX >= nextOpacityTFValue[0])
		{
			int newX = d2vX(nextOpacityTFValue[0]) - 1;
			setPoint(m_selectedPoint, newX, y);
			setColorPoint(m_selectedPoint, newX);
		}
		else if (dataX <= prevOpacityTFValue[0])
		{
			int newX = d2vX(prevOpacityTFValue[0]) + 1;
			setPoint(m_selectedPoint, newX, y);
			setColorPoint(m_selectedPoint, newX);
		}
		else
		{
			setPoint(m_selectedPoint, x, y);
			setColorPoint(m_selectedPoint, x);
		}
	}
	else
		setPointY(m_selectedPoint, y);

	triggerOnChange();
}

void iAChartTransferFunction::changeColor(QMouseEvent *event)
{
	if (event != NULL)
		m_selectedPoint = selectPoint(event);

	if (m_selectedPoint != -1)
	{
		double colorTFValue[6];
		m_colorTF->GetNodeValue(m_selectedPoint, colorTFValue);
		m_colorDlg->adjustSize();
		m_colorDlg->setCurrentColor(QColor(colorTFValue[1]*255, colorTFValue[2]*255, colorTFValue[3]*255));
		QRect scr = QApplication::desktop()->screenGeometry();
		m_colorDlg->setGeometry(scr.width()/2 - m_colorDlg->width()/2,
							scr.height()/2 - m_colorDlg->height()/2,
			m_colorDlg->width(),
			m_colorDlg->height());
		if (m_colorDlg->exec() == QDialog::Accepted)
		{
			QColor col = m_colorDlg->selectedColor();
			setColorPoint(m_selectedPoint, d2vX(colorTFValue[0]), (double)col.red()/255.0, (double)col.green()/255.0, (double)col.blue()/255.0 );
			m_colorTF->Modified();
			m_colorTF->Build();
			triggerOnChange();
		}
	}
}

bool iAChartTransferFunction::isEndPoint(int index)
{
	return index == 0 || index == m_opacityTF->GetSize()-1;
}

bool iAChartTransferFunction::isDeletable(int index)
{
	return !isEndPoint(index);
}

void iAChartTransferFunction::reset()
{
	if (m_opacityTF && m_colorTF)
	{
		m_opacityTF->RemoveAllPoints();
		m_opacityTF->AddPoint(chart->xBounds()[0], 0.0 );
		m_opacityTF->AddPoint(chart->xBounds()[1], 1.0 );
		m_colorTF->RemoveAllPoints();
		m_colorTF->AddRGBPoint(chart->xBounds()[0], 0.0, 0.0, 0.0 );
		m_colorTF->AddRGBPoint(chart->xBounds()[1], 1.0, 1.0, 1.0 );
		m_colorTF->Build();
		triggerOnChange();
	}
}

void iAChartTransferFunction::TranslateToNewRange(double const oldDataRange[2])
{
	double min, max;
	m_opacityTF->GetRange(min, max);
	for (int i = 0; i < m_opacityTF->GetSize(); i++)
	{
		double opacity[4];
		m_opacityTF->GetNodeValue(i, opacity);
		int opacityViewX = d2vX(opacity[0], oldDataRange[0], oldDataRange[1]);
		opacity[0] = v2dX(opacityViewX);
		m_opacityTF->SetNodeValue(i, opacity);
	}

	for (int i = 0; i < m_colorTF->GetSize(); i++)
	{
		double color[6];
		m_colorTF->GetNodeValue(i, color);
		int colorViewX = d2vX(color[0], oldDataRange[0], oldDataRange[1]);
		color[0] = v2dX(colorViewX);
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
	m_colorDlg->setCurrentColor(QColor(colorTFValue[1]*255, colorTFValue[2]*255, colorTFValue[3]*255));
	QRect scr = QApplication::desktop()->screenGeometry();
	m_colorDlg->adjustSize();
	m_colorDlg->setGeometry(scr.width()/2 - m_colorDlg->width()/2,
		scr.height()/2 - m_colorDlg->height()/2,
		m_colorDlg->width(),
		m_colorDlg->height());
	bool accepted = m_colorDlg->exec() == QDialog::Accepted;

	if (accepted)
	{
		QColor col = m_colorDlg->selectedColor();
		setColorPoint(m_selectedPoint, colorTFValue[0], (double)col.red()/255.0, (double)col.green()/255.0, (double)col.blue()/255.0 );
	}
	else if (m_selectedPoint > 0 && m_selectedPoint < m_opacityTF->GetSize()-1)
	{
		removePoint(m_selectedPoint);
	}
}

void iAChartTransferFunction::setColorPoint(int selectedPoint, double x, double red, double green, double blue)
{
	double colorVal[6] = { x, red, green, blue, 0.5, 0.0 };

	m_colorTF->SetNodeValue(selectedPoint, colorVal);
	m_colorTF->Modified();
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::setColorPoint(int selectedPoint, int x, double red, double green, double blue)
{
	double colorVal[6] = { v2dX(x), red, green, blue, 0.5, 0.0 };

	m_colorTF->SetNodeValue(selectedPoint, colorVal);
	m_colorTF->Modified();
	m_colorTF->Build();
	triggerOnChange();
}

void iAChartTransferFunction::setColorPoint(int selectedPoint, int x)
{
	double colorTFValue[6];
	m_colorTF->GetNodeValue(selectedPoint, colorTFValue);
	setColorPoint(selectedPoint, x, colorTFValue[1], colorTFValue[2], colorTFValue[3]);
}

void iAChartTransferFunction::setPoint(int selectedPoint, int x, int y)
{
	if (y < 0) y = 0;
	double opacityVal[4] = { v2dX(x), v2dY(y), 0.0, 0.0 };
	m_opacityTF->SetNodeValue(selectedPoint, opacityVal);
}

void iAChartTransferFunction::setPointX(int selectedPoint, int x)
{
	double opacityTFValues[4];
	m_opacityTF->GetNodeValue(selectedPoint, opacityTFValues);
	opacityTFValues[0] = v2dX(x);
	m_opacityTF->SetNodeValue(selectedPoint, opacityTFValues);
}

void iAChartTransferFunction::setPointY(int selectedPoint, int y)
{
	double opacityTFValues[4];
	m_opacityTF->GetNodeValue(selectedPoint, opacityTFValues);

	opacityTFValues[1] = v2dY(y);
	m_opacityTF->SetNodeValue(selectedPoint, opacityTFValues);
}

// TODO: unify somewhere!
double iAChartTransferFunction::v2dX(int x)
{
	double dX = ((double)(x-chart->xShift()) / (double)chart->activeWidth() * chart->xRange()) /chart->xZoom() + chart->xBounds()[0];
	return clamp(chart->xBounds()[0], chart->xBounds()[1], dX);
}

// convert from [0..maxDiagPixelHeight] to [0..1]
double iAChartTransferFunction::v2dY(int y)
{
	return mapToNorm(0, chart->chartHeight(), y);
}

int iAChartTransferFunction::d2vX(double x, double oldDataRange0, double oldDataRange1)
{
	if (oldDataRange0 == -1 && oldDataRange1 == -1)
		return (int)((x - chart->xBounds()[0]) * (double)chart->activeWidth() / chart->xRange()*chart->xZoom()) +chart->xShift();
	else
		return (int)((x -oldDataRange0) * (double)chart->activeWidth() / (oldDataRange1 - oldDataRange0)*chart->xZoom()) +chart->xShift();

}

// convert from [0..1] to [0..maxDiagPixelHeight]
int iAChartTransferFunction::d2vY(double y)
{
	return mapNormTo(0, std::max(0, chart->chartHeight()), y);
}

int iAChartTransferFunction::d2iX(double x)
{
	return d2vX(x) -chart->xShift();
}

int iAChartTransferFunction::d2iY(double y)
{
	return d2vY(y);
}

void iAChartTransferFunction::triggerOnChange()
{
	emit Changed();
}

void iAChartTransferFunction::enableRangeSliderHandles( bool rangeSliderHandles )
{
	m_rangeSliderHandles = rangeSliderHandles;
}
