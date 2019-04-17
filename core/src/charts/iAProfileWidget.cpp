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
#include "iAProfileWidget.h"

#include "iAMathUtility.h"

#include <QPainter>
#include <QToolTip>

#include <vtkPointData.h>
#include <vtkPolyData.h>

iAProfileWidget::iAProfileWidget(QWidget *parent, vtkPolyData* profData, double rayLength, QString yCapt, QString xCapt)
	: iAChartWidget(parent, xCapt, yCapt)
{
	initialize(profData, rayLength);
}

void iAProfileWidget::initialize(vtkPolyData* profData, double rayLength)
{
	profileData = profData;
	numBin = profileData->GetNumberOfPoints();
	rayLen = rayLength;
	profileData->GetScalarRange(yDataRange);
	scalars = profData->GetPointData()->GetScalars();
	yHeight = yDataRange[1] - yDataRange[0];
	setXBounds(0, rayLength);
	setYBounds(yDataRange[0], yDataRange[1]);
}

void iAProfileWidget::showDataTooltip(QMouseEvent *event)
{
	if (!scalars)
		return;
	int xPos = clamp(0, activeWidth() - 1, event->x() - leftMargin());
	int nthBin = (int)((((xPos-translationX) * numBin) / activeWidth()) / xZoom);
	double len = (((xPos-translationX) * rayLen) / activeWidth()) / xZoom;
	if (nthBin >= numBin || xPos == activeWidth()-1)
		nthBin = numBin-1;
	if (isTooltipShown())
	{
		QString text = xCaption
			+ QString(": %1").arg(len)
			+ "  "
			+ yCaption
			+ QString(": %1").arg(scalars->GetTuple1(nthBin));
		QToolTip::showText(event->globalPos(), text, this);
	}
	emit binSelected(nthBin);
}

void iAProfileWidget::drawPlots(QPainter &painter)
{
	if (!scalars)
		return;

	double binWidth = (double)(activeWidth()) / numBin *xZoom;
	int intBinWidth = (int)binWidth;
	if (intBinWidth < binWidth)
		intBinWidth++;
	painter.setPen(QWidget::palette().color(QPalette::Text));
	double scalingCoef = (double)(activeHeight()-1) / yHeight *yZoom;
	for ( int j = 0; j < numBin-1; j++ )
	{
		double x1 = (int)(j * binWidth);
		double y1 = (scalars->GetTuple1(j) - yDataRange[0]) * scalingCoef;
		double x2 = (int)((j+1) * binWidth);
		double y2 = (scalars->GetTuple1(j+1) - yDataRange[0]) * scalingCoef;
		painter.drawLine(x1, y1, x2, y2);
	}
}
