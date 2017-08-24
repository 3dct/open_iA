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
#include "iAHilbertLinePlots.h"
#include "iAColorTheme.h"

#include "defines.h"
#include "qcustomplot.h"

#include <QMap>
#include <QList>
#include <QColor>

const double golden_ratio = 0.618033988749895;

iAHilbertLinePlots::iAHilbertLinePlots( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: DatasetComparatorHLPConnector( parent, f )
{}

iAHilbertLinePlots::~iAHilbertLinePlots()
{}

void iAHilbertLinePlots::SetData(QMap<QString, QList<int> > datasetIntensityMap )
{
	m_DatasetIntensityMap = datasetIntensityMap;
}

void iAHilbertLinePlots::showHilbertLinePlots()
{
	QCustomPlot * customPlot = new QCustomPlot(HilbertLinePlots_dockWidgetContents);
	customPlot->legend->setVisible(true);
	customPlot->legend->setFont(QFont("Helvetica", 11));
	customPlot->xAxis->setLabel("Hilbert index");
	customPlot->yAxis->setLabel("Intensity valueis label");
	customPlot->xAxis2->setVisible(true);
	customPlot->xAxis2->setTickLabels(false);
	customPlot->yAxis2->setVisible(true);
	customPlot->yAxis2->setTickLabels(false);
	connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
	customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

	QMap<QString, QList<int>>::iterator it;
	iAColorTheme const * theme = iAColorThemeManager::GetInstance().GetTheme("Brewer Qualitaive 1 (max. 8)");
	QColor graphPenColor;
	QPen graphPen;
	graphPen.setWidth(3);
	int datasetIdx = 0;
	for (it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
	{
		if (m_DatasetIntensityMap.size() <= theme->size())
		{
			graphPenColor = theme->GetColor(datasetIdx);
		}
		else
		{
			//TODO: Use predefined colors, e.g., from color brewer or pantone
			// https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
			float h = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			h += golden_ratio;
			h = fmodf(h, 1.0);
			graphPenColor = QColor::fromHsvF(h, 0.95, 0.95, 1.0);
		}
		customPlot->addGraph();
		graphPen.setColor(graphPenColor);
		customPlot->graph(datasetIdx)->setPen(graphPen);
		customPlot->graph(datasetIdx)->setName(it.key());
		QList<int> l = it.value();
		QVector<double> x(l.size()), y(l.size());
		for (int i = 0; i < l.size(); ++i)
		{
			x[i] = i;
			y[i] = l[i];
		}
		customPlot->graph(datasetIdx)->setData(x, y);
		++datasetIdx;
	}
	customPlot->graph(0)->rescaleAxes();
	PlotsContainer_verticalLayout->addWidget(customPlot);
}