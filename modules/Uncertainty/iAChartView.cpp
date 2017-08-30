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
#include "iAChartView.h"

#include "qcustomplot.h"


iAChartView::iAChartView()
{
	m_plot = new QCustomPlot();

	setLayout(new QHBoxLayout());
	layout()->addWidget(m_plot);

	// generate some data:
	QVector<double> x(101), y(101); // initialize with entries 0..100
	for (int i = 0; i<101; ++i)
	{
		x[i] = i / 50.0 - 1; // x goes from -1 to 1
		y[i] = x[i] * x[i]; // let's plot a quadratic function
	}
	// create graph and assign data to it:
	m_plot->addGraph();
	m_plot->graph(0)->setData(x, y);
	m_plot->graph(0)->setLineStyle(QCPGraph::lsNone);
	m_plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle));

	// give the axes some labels:
	m_plot->xAxis->setLabel("x");
	m_plot->yAxis->setLabel("y");
	// set axes ranges, so we see all data:
	m_plot->xAxis->setRange(-1, 1);
	m_plot->yAxis->setRange(0, 1);
	m_plot->replot();
}
