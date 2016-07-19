/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#include "pch.h"
#include "iAProfileWidget.h"

#include <QMessageBox>
#include <QMdiSubWindow>
#include <QPainter>
#include <QToolTip>

#include <vtkPointData.h>

iAProfileWidget::iAProfileWidget(QWidget *parent, vtkPolyData* profData, double rayLength, QString yCapt, QString xCapt) 
	: iADiagramWidget (parent),
	xPos(0)
{
	min_intensity[0] = min_intensity[1] = min_intensity[2] = 0.0;
	max_intensity[0] = max_intensity[1] = max_intensity[2] = 0.0;
	scalars = 0;

	initialize(profData, rayLength);
	activeChild = parent->parentWidget();

	yCaption = yCapt;
	xCaption = xCapt;
}


void iAProfileWidget::initialize(vtkPolyData* profData, double rayLength)
{
	//initialise variables
	profileData = profData;

	numBin = profileData->GetNumberOfPoints();
	rayLen = rayLength;

	profileData->GetScalarRange(yDataRange);
	scalars = profData->GetPointData()->GetScalars();

	yHeight = yDataRange[1] - yDataRange[0];

	//set attribut, so that the objects are deleted while
	//this widget is closed
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setFocusPolicy(Qt::WheelFocus);

	mode = NO_MODE;

	draw = false;
}


void iAProfileWidget::paintEvent(QPaintEvent * )
{
	if (draw) this->drawProfilePlot();
	//set the widget as the drawing device for the Qpainter painter
	QPainter painter(this);
	//use the painter to draw from Qimage image to the drawing device widget
	painter.drawImage(QRectF(0, 0, width, height), image);
}


void iAProfileWidget::drawProfilePlot()
{	
	//initialise a painter
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);

	drawBackground(painter);

	//change the origin of the window to left bottom
	painter.translate(translationX+getLeftMargin(), height-getBottomMargin());
	painter.scale(1, -1);

	drawHistogram(painter);

	//draw functions
	painter.scale(1, -1);
	painter.setRenderHint(QPainter::Antialiasing, false);

	drawAxes(painter);


	//update the painter
	update();
	draw = false;
}


void iAProfileWidget::redraw()
{
	draw = true;
	update();
}


void iAProfileWidget::selectBin(QMouseEvent *event)
{
	if (!scalars)
	{
		return;
	}
	xPos = event->x() - getLeftMargin();
	if (xPos < 0) 
		xPos = 0;
	if (xPos >= getActiveWidth()) 
		xPos = getActiveWidth()-1;

	//calculate the nth bin located at a given pixel, actual formula is (i/100 * width) * (numBin / width)
	int nthBin = (int)((((xPos-translationX) * numBin) / getActiveWidth()) / xZoom);
	double len = (((xPos-translationX) * rayLen) / getActiveWidth()) / xZoom;
	if (nthBin >= numBin || xPos == getActiveWidth()-1)
	{
		nthBin = numBin-1;
	}

	QString text = xCaption 
		+ QString(": %1").arg(len)
		+ "  " 
		+ yCaption 
		+ QString(": %1").arg(scalars->GetTuple1(nthBin));

	QToolTip::showText( event->globalPos(), text, this );
	emit binSelected(nthBin);
}


void iAProfileWidget::drawHistogram(QPainter &painter)
{
	double binWidth = (double)(getActiveWidth()) / numBin *xZoom;

	int intBinWidth = (int)binWidth;

	if (intBinWidth < binWidth)
		intBinWidth++;

	//change the pen color to blue
	painter.setPen(QColor(70,70,70,255));

	if (scalars)
	{
		//draw the histogram using the painter on the image
		double scalingCoef = (double)(height-getBottomMargin()-1) / yHeight *yZoom;
		for ( int j = 0; j < numBin-1; j++ ) 
		{		 
			double x1 = (int)(j * binWidth);
			double y1 = (scalars->GetTuple1(j) - yDataRange[0]) * scalingCoef;
			double x2 = (int)((j+1) * binWidth);
			double y2 = (scalars->GetTuple1(j+1) - yDataRange[0]) * scalingCoef;

			painter.drawLine(x1, y1, x2, y2);// draw line
			//painter.fillRect(QRect(x1, 0, intBinWidth, y1), Qt::blue); // draw line 
		}
	}
}


void iAProfileWidget::drawAxes(QPainter &painter)
{
	drawXAxis(painter);
	drawYAxis(painter);
}


void iAProfileWidget::drawXAxis(QPainter &painter)
{
	painter.setPen(Qt::black);

	//markings in x axis
	int i = 0;
	int stepNumber = (int)(10.0*xZoom);
	double step = 1.0 / stepNumber;
	QFontMetrics fm = painter.fontMetrics();
	while (i <= stepNumber)
	{
		//calculate the nth bin located at a given pixel, actual formula is (i/100 * width) * (numBin / width)
		double pos = step * i;
		int nthBin = (int)(pos * rayLen);
		//get the intensity value into a string
		double value = nthBin;

		QString text;
		if (value < 1.0)
			text = QString::number(value, 'g', 3);
		else
			text = QString::number((int)value, 10);

		//calculate the x coordinate
		int x = (int)(pos * getActiveWidth() * xZoom);
		//draw a small indicator line
		painter.drawLine(x, (int)(getBottomMargin()*0.1), x, -1);

		if (i <= 0)
		{
			painter.drawText(x, TEXT_Y, text ); //write the text right aligned to indicator line
		}
		else if(stepNumber == i)
		{
			painter.drawText(x - fm.width(text), TEXT_Y, text); //write the text left aligned to the indicator line
		}
		else
		{
			painter.drawText(x - 0.5*fm.width(text), TEXT_Y, text); //write the text centered to the indicator line
		}
		i++;
	}
	//draw the x axis
	painter.drawLine(0, -1, (int)(getActiveWidth()*xZoom), -1);
	//write the x axis label
	painter.drawText( QPointF((int)(getActiveWidth() * 0.45 ), getBottomMargin()-2), xCaption);
}


void iAProfileWidget::drawYAxis(QPainter &painter)
{
	//change pen color to black
	painter.setPen(Qt::black);
	QFontMetrics fm = painter.fontMetrics();
	int fontHeight = fm.height();

	//markings in y axis
	int i = 0;
	int stepNumber = 5;
	double step = 1.0 / (stepNumber*yZoom);
	while (i <= stepNumber)
	{
		//calculate the nth bin located at a given pixel, actual formula is (i/100 * width) * (rayLength / width)
		double pos = step * i;
		//get the intensity value into a string
		double yValue = pos * yHeight + yDataRange[0];

		QString text;
		if (yValue < 1.0)
			text = QString::number(yValue, 'g', 3);
		else
			text = QString::number((int)yValue, 10);

		//calculate the y coordinate
		int y = -(int)(pos * getActiveHeight() * yZoom)-1;
		//draw a small indicator line
		painter.drawLine((int)(-getLeftMargin()*0.1), y, 0, y);

		if(i == stepNumber)
			painter.drawText(TEXT_X-getLeftMargin(), y+0.6*fontHeight, text); //write the text left aligned to the indicator line
		else
			painter.drawText(TEXT_X-getLeftMargin(), y+0.25*fontHeight, text); //write the text centered to the indicator line

		i++;
	}
	painter.drawLine(0, -1, 0, -(int)(getActiveHeight()*yZoom));
	//write the y axis label
	painter.save();
	painter.rotate(-90);
	painter.drawText( 
		QPointF(
		getActiveHeight()*0.5 - 0.5*fm.width(yCaption), 
		-getLeftMargin() + fontHeight - 5), 
		yCaption);
	painter.restore();
}
