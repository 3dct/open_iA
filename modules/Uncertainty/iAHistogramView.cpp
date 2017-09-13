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
#include "iAHistogramView.h"

#include "iAFunctionDrawers.h"
#include "iASimpleHistogramData.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

#include <QHBoxLayout>

iAHistogramChartWidget::iAHistogramChartWidget(QSharedPointer<iASimpleHistogramData> data, QString const & caption):
	iADiagramFctWidget(nullptr, nullptr, vtkSmartPointer<vtkPiecewiseFunction>(), vtkSmartPointer<vtkColorTransferFunction>(), caption),
m_data(data) {
}

QSharedPointer<iAAbstractDiagramRangedData> iAHistogramChartWidget::GetData()
{
	return m_data;
}

QSharedPointer<iAAbstractDiagramRangedData> const iAHistogramChartWidget::GetData() const
{
	return m_data;
}
QSharedPointer<iAAbstractDrawableFunction> iAHistogramChartWidget::CreatePrimaryDrawer()
{
	return QSharedPointer<iAAbstractDrawableFunction>(new iABarGraphDrawer(m_data, QColor(0, 0, 255), 2));
}

iAHistogramView::iAHistogramView()
{
	setLayout(new QHBoxLayout());
}

void iAHistogramView::AddChart(QString const & caption, QSharedPointer<iASimpleHistogramData> data)
{
	m_chart = new iAHistogramChartWidget(data, caption);
	layout()->addWidget(m_chart);
}
