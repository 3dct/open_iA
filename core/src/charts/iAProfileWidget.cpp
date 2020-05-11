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
#include "iAProfileWidget.h"

#include "iAMathUtility.h"

#include <QPainter>
#include <QToolTip>

#include <vtkPointData.h>
#include <vtkPolyData.h>

iAProfileWidget::iAProfileWidget(QWidget *parent, vtkPolyData* profData, double rayLength, QString yCapt, QString xCapt):
	iAChartWidget(parent, xCapt, yCapt),
	m_profileData(nullptr),
	m_scalars(nullptr)
{
	initialize(profData, rayLength);
}

void iAProfileWidget::initialize(vtkPolyData* profData, double rayLength)
{
	m_profileData = profData;
	m_numBin = m_profileData->GetNumberOfPoints();
	m_rayLen = rayLength;
	double yDataRange[2];
	m_profileData->GetScalarRange(yDataRange);
	m_scalars = profData->GetPointData()->GetScalars();
	m_yHeight = yDataRange[1] - yDataRange[0];
	if (m_yHeight == 0)
	{
		m_yHeight = 1;
		yDataRange[1] = yDataRange[0] + m_yHeight;
	}
	setXBounds(0, rayLength);
	setYBounds(yDataRange[0], yDataRange[1]);
}

void iAProfileWidget::showDataTooltip(QHelpEvent *event)
{
	if (!m_scalars)
	{
		return;
	}
	int xPos = clamp(0, activeWidth() - 1, event->x() - leftMargin());
	int nthBin = static_cast<int>((((xPos-m_translationX) * m_numBin) / activeWidth()) / m_xZoom);
	double len = (((xPos-m_translationX) * m_rayLen) / activeWidth()) / m_xZoom;
	if (nthBin >= m_numBin || xPos == activeWidth() - 1)
	{
		nthBin = m_numBin - 1;
	}
	if (isTooltipShown())
	{
		QString text = m_xCaption + QString(": %1").arg(len)
		      + "  " + m_yCaption + QString(": %1").arg(m_scalars->GetTuple1(nthBin));
		QToolTip::showText(event->globalPos(), text, this);
	}
	emit binSelected(nthBin);
}

void iAProfileWidget::drawPlots(QPainter &painter)
{
	if (!m_scalars)
	{
		return;
	}
	double binWidth = (double)(activeWidth()) / m_numBin * m_xZoom;
	painter.setPen(QWidget::palette().color(QPalette::Text));
	double scalingCoef = (double)(activeHeight()-1) / m_yHeight * m_yZoom;
	for ( int j = 0; j < m_numBin-1; j++ )
	{
		double x1 = (int)(j * binWidth);
		double y1 = (m_scalars->GetTuple1(j) - yBounds()[0]) * scalingCoef;
		double x2 = (int)((j+1) * binWidth);
		double y2 = (m_scalars->GetTuple1(j+1) - yBounds()[0]) * scalingCoef;
		painter.drawLine(x1, y1, x2, y2);
	}
}
