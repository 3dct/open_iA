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
#include "dlg_transfer.h"

#include "iAConsole.h"
#include "iADiagramFctWidget.h"
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

dlg_transfer::dlg_transfer(iADiagramFctWidget *fctDiagram, QColor color):
	dlg_function(fctDiagram),
	m_rangeSliderHandles(false)
{
	this->color = color;
	
	dlg = new QColorDialog(fctDiagram);
	
	gradient.setSpread(QGradient::PadSpread);

	opacityTF = NULL;
	colorTF = NULL;

	selectedPoint = -1;
}

dlg_transfer::~dlg_transfer()
{
	delete dlg;
}

void dlg_transfer::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (fctDiagram->getSelectedFunction() == this);

	QPen pen = painter.pen();
	pen.setColor(color); pen.setWidth(lineWidth);
	QPen pen1 = painter.pen();
	pen1.setColor(color); pen1.setWidth(1);
	QPen redPen = painter.pen();
	redPen.setColor(Qt::red); redPen.setWidth(1);

	painter.setPen(pen);
	painter.setBrush(QColor(128, 128, 128, 255));

	QColor c;
	double gradientWidth = fctDiagram->getActiveWidth()*fctDiagram->getZoom();

	gradient = QLinearGradient();
	gradient.setStart(0, 0);
	gradient.setFinalStop(gradientWidth, 0);
	
	// draw opacity and color tf
	if (opacityTF->GetSize() != colorTF->GetSize())
	{
		DEBUG_LOG(QString("Definition mismatch: opacity TF has %1 positions, color TF %2!")
			.arg(opacityTF->GetSize())
			.arg(colorTF->GetSize()));
		return;
	}
	double opacityTFValue[4];
	double colorTFValue[6];

	opacityTF->GetNodeValue(0, opacityTFValue);
	colorTF->GetNodeValue(0, colorTFValue);
		
	int x1, y1;
	x1 = d2iX(opacityTFValue[0]);
	y1 = d2iY(opacityTFValue[1]);
		
	c = QColor(colorTFValue[1]*255.0, colorTFValue[2]*255.0, colorTFValue[3]*255.0, 150); 
	gradient.setColorAt((double)x1/gradientWidth, c );
		
	for ( int i = 1; i < opacityTF->GetSize(); i++)
	{
		opacityTF->GetNodeValue(i, opacityTFValue);
		colorTF->GetNodeValue(i, colorTFValue);

		int x2 = d2iX(opacityTFValue[0]);
		int y2 = d2iY(opacityTFValue[1]);
		painter.drawLine(x1, y1, x2, y2); // draw line
		if (active)
		{
			if (!m_rangeSliderHandles)
			{
				if (i - 1 == selectedPoint)
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
				if ( i - 1 == selectedPoint &&  i - 1 > 0 )
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
		gradient.setColorAt((double)x2/gradientWidth, QColor(c.red(), c.green(), c.blue(), 150));
		x1 = x2;
		y1 = y2;
	}
	if ( active && !m_rangeSliderHandles )
	{
		if (selectedPoint == opacityTF->GetSize()-1)
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

void dlg_transfer::draw(QPainter &painter)
{
	draw(painter, color, 1);
}

void dlg_transfer::drawOnTop(QPainter &painter)
{
	if ( opacityTF->GetSize() == colorTF->GetSize())
	{
		double gradientWidth = fctDiagram->getActiveWidth()*fctDiagram->getZoom();

		painter.fillRect( 0, 0, gradientWidth, -fctDiagram->GetTFGradientHeight(), gradient );
	}
}

int dlg_transfer::selectPoint(QMouseEvent *event, int *x)
{ 
	int lx = event->x() - fctDiagram->getLeftMargin();
	int ly = fctDiagram->geometry().height() - event->y() - fctDiagram->getBottomMargin() - fctDiagram->getTranslationY();
	int index = -1;
	
	double pointValue[4];
	for (int pointIndex = 0; pointIndex < opacityTF->GetSize(); pointIndex++)
	{
		opacityTF->GetNodeValue(pointIndex, pointValue);
		int viewX, viewY;
		
		viewX = d2vX(pointValue[0]);
		viewY = d2vY(pointValue[1]);
		
		if ( !m_rangeSliderHandles )
		{
			if ( ( pointIndex == selectedPoint &&
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

	selectedPoint = index;

	return index;
}

int dlg_transfer::addPoint(int x, int y)
{
	if (y < 0)
		y = 0;

	double d[2] = { v2dX(x), v2dY(y) };
	selectedPoint = opacityTF->AddPoint(d[0], d[1]);

	return selectedPoint;
}

void dlg_transfer::addColorPoint(int x, double red, double green, double blue)
{
	if (red < 0 || green < 0 || blue < 0)
	{
		QGradientStops stops = gradient.stops();
		double gradientWidth = fctDiagram->getActiveWidth()*fctDiagram->getZoom();
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
	
	colorTF->AddRGBPoint(v2dX(x), red, green, blue);
	colorTF->Build();
	triggerOnChange();
}

void dlg_transfer::removePoint(int index)
{
	if (opacityTF->GetSize()>2)
	{
		double values[4];
		opacityTF->GetNodeValue(index, values);

		double cvalues[6];
		colorTF->GetNodeValue(index, cvalues);
		opacityTF->RemovePoint(values[0]);
		colorTF->RemovePoint(values[0]);
		colorTF->Build();
		triggerOnChange();
	}
}

void dlg_transfer::moveSelectedPoint(int x, int y)
{
	if (x > fctDiagram->getActiveWidth()-1) x = fctDiagram->getActiveWidth() - 1;
	if (y < 0) y = 0;
	if (y > fctDiagram->geometry().height() - fctDiagram->getBottomMargin()-1) y = fctDiagram->geometry().height() - fctDiagram->getBottomMargin()-1;

	double dataX = v2dX(x);
	if (selectedPoint != 0 && selectedPoint != opacityTF->GetSize()-1)
	{
		double nextOpacityTFValue[4];
		double prevOpacityTFValue[4];
		
		opacityTF->GetNodeValue(selectedPoint+1, nextOpacityTFValue);
		opacityTF->GetNodeValue(selectedPoint-1, prevOpacityTFValue);
		
		if (dataX >= nextOpacityTFValue[0])
		{
			int newX = d2vX(nextOpacityTFValue[0]) - 1;
			setPoint(selectedPoint, newX, y);
			setColorPoint(selectedPoint, newX);
		}
		else if (dataX <= prevOpacityTFValue[0])
		{
			int newX = d2vX(prevOpacityTFValue[0]) + 1;
			setPoint(selectedPoint, newX, y);
			setColorPoint(selectedPoint, newX);
		}
		else
		{
			setPoint(selectedPoint, x, y);
			setColorPoint(selectedPoint, x);
		}
	}
	else
		setPointY(selectedPoint, y);

	triggerOnChange();
}

void dlg_transfer::changeColor(QMouseEvent *event)
{
	if (event != NULL)
		selectedPoint = selectPoint(event);
	
	if (selectedPoint != -1)
	{
		double colorTFValue[6];
		colorTF->GetNodeValue(selectedPoint, colorTFValue);
		dlg->adjustSize();
		dlg->setCurrentColor(QColor(colorTFValue[1]*255, colorTFValue[2]*255, colorTFValue[3]*255));
		QRect scr = QApplication::desktop()->screenGeometry();
		dlg->setGeometry(scr.width()/2 - dlg->width()/2,
							scr.height()/2 - dlg->height()/2,
							dlg->width(),
							dlg->height());
		if (dlg->exec() == QDialog::Accepted)
		{
			QColor col = dlg->selectedColor();
			setColorPoint(selectedPoint, d2vX(colorTFValue[0]), (double)col.red()/255.0, (double)col.green()/255.0, (double)col.blue()/255.0 );
			colorTF->Modified();
			colorTF->Build();
			triggerOnChange();
		}
	}
}

bool dlg_transfer::isEndPoint(int index)
{
	return index == 0 || index == opacityTF->GetSize()-1;
}

bool dlg_transfer::isDeletable(int index)
{
	return !isEndPoint(index);
}

void dlg_transfer::reset()
{
	if (opacityTF != NULL && colorTF != NULL)
	{
		double dataRange[2];
		fctDiagram->GetDataRange(dataRange);

		opacityTF->RemoveAllPoints();
		opacityTF->AddPoint( dataRange[0], 0.0 );
		opacityTF->AddPoint( dataRange[1], 1.0 );
		colorTF->RemoveAllPoints();
		colorTF->AddRGBPoint( dataRange[0], 0.0, 0.0, 0.0 );
		colorTF->AddRGBPoint( dataRange[1], 1.0, 1.0, 1.0 );
		colorTF->Build();
		triggerOnChange();
	}
}

void dlg_transfer::TranslateToNewRange(double oldDataRange[2])
{
	double min, max;
	opacityTF->GetRange(min, max);
	for (int i = 0; i < opacityTF->GetSize(); i++)
	{
		double opacity[4];
		opacityTF->GetNodeValue(i, opacity);
		int opacityViewX = d2vX(opacity[0], oldDataRange[0], oldDataRange[1]);
		opacity[0] = v2dX(opacityViewX);
		opacityTF->SetNodeValue(i, opacity);
	}

	for (int i = 0; i < colorTF->GetSize(); i++)
	{
		double color[6];
		colorTF->GetNodeValue(i, color);
		int colorViewX = d2vX(color[0], oldDataRange[0], oldDataRange[1]);
		color[0] = v2dX(colorViewX);
		colorTF->SetNodeValue(i, color);
	}
	colorTF->Modified();
	colorTF->Build();
	triggerOnChange();
}

void dlg_transfer::mouseReleaseEventAfterNewPoint(QMouseEvent *)
{
	double colorTFValue[6];
	colorTF->GetNodeValue(selectedPoint, colorTFValue);
	dlg->setCurrentColor(QColor(colorTFValue[1]*255, colorTFValue[2]*255, colorTFValue[3]*255));
	QRect scr = QApplication::desktop()->screenGeometry();
	dlg->adjustSize();
	dlg->setGeometry(scr.width()/2 - dlg->width()/2,
		scr.height()/2 - dlg->height()/2,
		dlg->width(),
		dlg->height());
	bool accepted = dlg->exec() == QDialog::Accepted;
	
	if (accepted)
	{
		QColor col = dlg->selectedColor();
		setColorPoint(selectedPoint, colorTFValue[0], (double)col.red()/255.0, (double)col.green()/255.0, (double)col.blue()/255.0 );
	}
	else if (selectedPoint > 0 && selectedPoint < opacityTF->GetSize()-1)
	{
		removePoint(selectedPoint);
	}
}

void dlg_transfer::setColorPoint(int selectedPoint, double x, double red, double green, double blue)
{
	double colorVal[6] = { x, red, green, blue, 0.5, 0.0 };

	colorTF->SetNodeValue(selectedPoint, colorVal);
	colorTF->Modified();
	colorTF->Build();
	triggerOnChange();
}

void dlg_transfer::setColorPoint(int selectedPoint, int x, double red, double green, double blue)
{
	double colorVal[6] = { v2dX(x), red, green, blue, 0.5, 0.0 };

	colorTF->SetNodeValue(selectedPoint, colorVal);
	colorTF->Modified();
	colorTF->Build();
	triggerOnChange();
}

void dlg_transfer::setColorPoint(int selectedPoint, int x)
{
	double colorTFValue[6];
	colorTF->GetNodeValue(selectedPoint, colorTFValue);
	setColorPoint(selectedPoint, x, colorTFValue[1], colorTFValue[2], colorTFValue[3]);
}

void dlg_transfer::setPoint(int selectedPoint, int x, int y)
{
	if (y < 0) y = 0;
	double opacityVal[4] = { v2dX(x), v2dY(y), 0.0, 0.0 };
	opacityTF->SetNodeValue(selectedPoint, opacityVal);
}

void dlg_transfer::setPointX(int selectedPoint, int x)
{
	double opacityTFValues[4];
	opacityTF->GetNodeValue(selectedPoint, opacityTFValues);
	opacityTFValues[0] = v2dX(x);
	opacityTF->SetNodeValue(selectedPoint, opacityTFValues);
}

void dlg_transfer::setPointY(int selectedPoint, int y)
{
	double opacityTFValues[4];
	opacityTF->GetNodeValue(selectedPoint, opacityTFValues);
	
	opacityTFValues[1] = v2dY(y);
	opacityTF->SetNodeValue(selectedPoint, opacityTFValues);
}

double dlg_transfer::v2dX(int x)
{
	double dataRange[2];
	fctDiagram->GetDataRange(dataRange);

	double dX = ((double)(x-fctDiagram->getTranslationX()) /
		(double)fctDiagram->getActiveWidth() * (dataRange[1] - dataRange[0]) ) /fctDiagram->getZoom() + dataRange[0];

	return clamp(dataRange[0], dataRange[1], dX);
}

// convert from [0..maxDiagPixelHeight] to [0..1]
double dlg_transfer::v2dY(int y)
{
	return mapToNorm(0, fctDiagram->getChartHeight(), y);
}

int dlg_transfer::d2vX(double x, double oldDataRange0, double oldDataRange1)
{
	double dataRange[2];
	fctDiagram->GetDataRange(dataRange);

	if (oldDataRange0 == -1 && oldDataRange1 == -1)
		return (int)((x -dataRange[0]) * (double)fctDiagram->getActiveWidth() / (dataRange[1] - dataRange[0])*fctDiagram->getZoom()) +fctDiagram->getTranslationX();
	else
		return (int)((x -oldDataRange0) * (double)fctDiagram->getActiveWidth() / (oldDataRange1 - oldDataRange0)*fctDiagram->getZoom()) +fctDiagram->getTranslationX();
		
}

// convert from [0..1] to [0..maxDiagPixelHeight]
int dlg_transfer::d2vY(double y)
{
	return mapNormTo(0, std::max(0, fctDiagram->getChartHeight()), y);;
}

int dlg_transfer::d2iX(double x)
{
	return d2vX(x) -fctDiagram->getTranslationX();
}

int dlg_transfer::d2iY(double y)
{
	return d2vY(y);
}

void dlg_transfer::triggerOnChange()
{
	emit Changed();
}

void dlg_transfer::loadTransferFunction(QDomNode &functionsNode, double range[2])
{
	QDomNode transferElement = functionsNode.namedItem("transfer");
	if (!transferElement.isElement())
		return;

	// does functions node exist
	double value, opacity, red, green, blue;

	GetOpacityFunction()->RemoveAllPoints();
	GetColorFunction()->RemoveAllPoints();

	QDomNodeList list = transferElement.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);

		QDomNamedNodeMap attributes = node.attributes();
		value = attributes.namedItem("value").nodeValue().toDouble();
		opacity = attributes.namedItem("opacity").nodeValue().toDouble();
		red = attributes.namedItem("red").nodeValue().toDouble();
		green = attributes.namedItem("green").nodeValue().toDouble();
		blue = attributes.namedItem("blue").nodeValue().toDouble();

		if (value < range[0]) value = range[0];
		if (value > range[1]) value = range[1];
		GetOpacityFunction()->AddPoint(value, opacity);
		GetColorFunction()->AddRGBPoint(value, red, green, blue);
	}
	triggerOnChange();
}

void dlg_transfer::enableRangeSliderHandles( bool rangeSliderHandles )
{
	m_rangeSliderHandles = rangeSliderHandles;
}
