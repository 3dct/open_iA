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

#include "iAConsole.h"
#include "qcustomplot.h"
#include "iAToolsVTK.h"
#include "iAColors.h"

#include <vtkImageData.h>


iAChartView::iAChartView()
{
	m_plot = new QCustomPlot();
	m_plot->setInteraction(QCP::iRangeDrag, true);
	m_plot->setInteraction(QCP::iRangeZoom, true);
	m_plot->setInteraction(QCP::iMultiSelect, true);
	m_plot->setMultiSelectModifier(Qt::ShiftModifier);
	m_plot->setInteraction(QCP::iSelectPlottables, true);
	connect(m_plot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(chartMousePress(QMouseEvent *)));
	setLayout(new QHBoxLayout());
	layout()->addWidget(m_plot);
}


void iAChartView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
	int * dim = imgX->GetDimensions();
	m_voxelCount = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
	QVector<double> x, y, t;
	x.reserve(m_voxelCount);
	y.reserve(m_voxelCount);
	t.reserve(m_voxelCount);
	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	std::copy(bufX, bufX + m_voxelCount, std::back_inserter(x));
	std::copy(bufY, bufY + m_voxelCount, std::back_inserter(y));
	for (int i = 0; i < m_voxelCount; ++i)
	{	// unfortunately we seem to require this additional storage
		t.push_back(i);  // to make QCustomPlot not sort the data
	}

	auto curve = new QCPCurve(m_plot->xAxis, m_plot->yAxis);
	curve->setData(t, x, y, true);
	curve->setLineStyle(QCPCurve::lsNone);
	curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Uncertainty::ChartColor, 2));
	curve->setSelectable(QCP::stMultipleDataRanges);
	curve->selectionDecorator()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Uncertainty::SelectionColor, 2));
	connect(curve, SIGNAL(selectionChanged(QCPDataSelection const &)), this, SLOT(selectionChanged(QCPDataSelection const &)));

	m_plot->xAxis->setLabel(captionX);
	m_plot->yAxis->setLabel(captionY);
	m_plot->xAxis->setRange(0, 1);
	m_plot->yAxis->setRange(0, 1);
	m_plot->replot();

	m_selectionImg = AllocateImage(imgX);
}


void iAChartView::selectionChanged(QCPDataSelection const & selection)
{
	DEBUG_LOG("Selection Changed");
	double* buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int v=0; v<m_voxelCount; ++v)
	{
		*buf = 0;
		buf++;
	}
	buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int r = 0; r < selection.dataRangeCount(); ++r)
	{
		std::fill(buf + selection.dataRange(r).begin(), buf + selection.dataRange(r).end(), 1);
	}
	m_selectionImg->Modified();

	StoreImage(m_selectionImg, "C:/Users/p41143/selection.mhd", true);
	emit SelectionChanged();
}


vtkImagePointer iAChartView::GetSelectionImage()
{
	return m_selectionImg;
}


void iAChartView::chartMousePress(QMouseEvent *)
{
	if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
	{	// allow selection with Ctrl key
		m_plot->setSelectionRectMode(QCP::srmSelect);
	}
	else
	{	// enable dragging otherwise
		m_plot->setSelectionRectMode(QCP::srmNone);
	}
}
