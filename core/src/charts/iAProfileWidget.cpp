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
#include "iAProfileWidget.h"

#include "iAMathUtility.h"

#include <QMessageBox>
#include <QPainter>
#include <QToolTip>

#include <vtkPointData.h>

iAProfileWidget::iAProfileWidget(QWidget *parent, vtkPolyData* profData, double rayLength, QString yCapt, QString xCapt) 
	: iADiagramWidget (parent),
	xPos(0),
	scalars(nullptr),
	xCaption(xCapt),
	yCaption(yCapt)
{
	min_intensity[0] = min_intensity[1] = min_intensity[2] = 0.0;
	max_intensity[0] = max_intensity[1] = max_intensity[2] = 0.0;
	initialize(profData, rayLength);
	activeChild = parent->parentWidget();
}


void iAProfileWidget::initialize(vtkPolyData* profData, double rayLength)
{
	profileData = profData;
	numBin = profileData->GetNumberOfPoints();
	rayLen = rayLength;
	profileData->GetScalarRange(yDataRange);
	scalars = profData->GetPointData()->GetScalars();
	yHeight = yDataRange[1] - yDataRange[0];
	mode = NO_MODE;
	draw = false;
}


void iAProfileWidget::paintEvent(QPaintEvent * )
{
	if (draw)
		this->drawProfilePlot();
	QPainter painter(this);
	painter.drawImage(QRectF(0, 0, width, height), image);
}


void iAProfileWidget::drawProfilePlot()
{
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	drawBackground(painter);

	//change the origin of the window to left bottom
	painter.translate(translationX+LeftMargin(), height-BottomMargin());
	painter.scale(1, -1);

	drawHistogram(painter);

	painter.scale(1, -1);
	painter.setRenderHint(QPainter::Antialiasing, false);

	drawAxes(painter);

	update();
	draw = false;
}


void iAProfileWidget::redraw()
{
	draw = true;
	update();
}


void iAProfileWidget::showDataTooltip(QMouseEvent *event)
{
	if (!scalars)
		return;

	xPos = clamp(0, ActiveWidth() - 1, event->x() - LeftMargin());
	int nthBin = (int)((((xPos-translationX) * numBin) / ActiveWidth()) / xZoom);
	double len = (((xPos-translationX) * rayLen) / ActiveWidth()) / xZoom;
	if (nthBin >= numBin || xPos == ActiveWidth()-1)
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
	if (!scalars)
		return;

	double binWidth = (double)(ActiveWidth()) / numBin *xZoom;
	int intBinWidth = (int)binWidth;
	if (intBinWidth < binWidth)
		intBinWidth++;
	painter.setPen(QWidget::palette().color(QPalette::Text));
	double scalingCoef = (double)(height-BottomMargin()-1) / yHeight *yZoom;
	for ( int j = 0; j < numBin-1; j++ )
	{
		double x1 = (int)(j * binWidth);
		double y1 = (scalars->GetTuple1(j) - yDataRange[0]) * scalingCoef;
		double x2 = (int)((j+1) * binWidth);
		double y2 = (scalars->GetTuple1(j+1) - yDataRange[0]) * scalingCoef;
		painter.drawLine(x1, y1, x2, y2);
	}
}


void iAProfileWidget::drawAxes(QPainter &painter)
{
	drawXAxis(painter);
	drawYAxis(painter);
}


void iAProfileWidget::drawXAxis(QPainter &painter)
{
	painter.setPen(QWidget::palette().color(QPalette::Text));
	int i = 0;
	int stepNumber = (int)(10.0*xZoom);
	double step = 1.0 / stepNumber;
	QFontMetrics fm = painter.fontMetrics();
	while (i <= stepNumber)
	{
		double pos = step * i;
		int nthBin = (int)(pos * rayLen);
		double value = nthBin;
		QString text = (value < 1.0) ? QString::number(value, 'g', 3) : QString::number((int)value, 10);
		int x = (int)(pos * ActiveWidth() * xZoom);
		painter.drawLine(x, (int)(BottomMargin()*0.1), x, -1);		// draw a small indicator line
		if (i <= 0)
			painter.drawText(x, TEXT_Y, text );						// write the text right aligned to indicator line
		else if(stepNumber == i)
			painter.drawText(x - fm.width(text), TEXT_Y, text);		// write the text left aligned to the indicator line
		else
			painter.drawText(x - 0.5*fm.width(text), TEXT_Y, text); // write the text centered to the indicator line
		i++;
	}
	painter.drawLine(0, -1, (int)(ActiveWidth()*xZoom), -1);		// draw the x axis
	painter.drawText(												// write the x axis label
		QPointF((int)(ActiveWidth() * 0.45), BottomMargin() - 2),
		xCaption);
}


void iAProfileWidget::drawYAxis(QPainter &painter)
{
	painter.setPen(QWidget::palette().color(QPalette::Text));
	QFontMetrics fm = painter.fontMetrics();
	int fontHeight = fm.height();
	int i = 0;
	int stepNumber = 5;
	double step = 1.0 / (stepNumber*yZoom);
	while (i <= stepNumber)
	{
		double pos = step * i;
		double yValue = pos * yHeight + yDataRange[0];
		QString text = (yValue < 1.0) ? QString::number(yValue, 'g', 3) : QString::number((int)yValue, 10);
		int y = -(int)(pos * ActiveHeight() * yZoom)-1;
		painter.drawLine((int)(-LeftMargin()*0.1), y, 0, y);				// draw a small indicator line

		if(i == stepNumber)
			painter.drawText(TEXT_X-LeftMargin(), y+0.6*fontHeight, text);	// write the text left aligned to the indicator line
		else
			painter.drawText(TEXT_X-LeftMargin(), y+0.25*fontHeight, text);	// write the text centered to the indicator line
		i++;
	}
	painter.drawLine(0, -1, 0, -(int)(ActiveHeight()*yZoom));				// draw the y axis
	painter.save();
	painter.rotate(-90);
	painter.drawText(														// write the y axis label
		QPointF(
			ActiveHeight()*0.5 - 0.5*fm.width(yCaption),
			-LeftMargin() + fontHeight - 5),
		yCaption);
	painter.restore();
}
