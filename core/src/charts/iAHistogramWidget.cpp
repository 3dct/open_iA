/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAHistogramWidget.h"

#include "dlg_function.h"
#include "iAHistogramData.h"
#include "iAPlotTypes.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>


iAHistogramWidget::iAHistogramWidget(QWidget *parent,
	MdiChild * mdiChild,
	vtkPiecewiseFunction* oTF,
	vtkColorTransferFunction* cTF,
	iAPlotData::DataType* histData,
	iAPlotData::DataType dataMin,
	iAPlotData::DataType dataMax,
	int bins,
	double space,
	QString const & label,
	bool reset)
	: iADiagramFctWidget(parent, mdiChild, label)
{
	SetTransferFunctions(cTF, oTF);
	datatypehistograminitialize(histData, reset, dataMin, dataMax, bins, space);
	AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(m_data, QColor(70, 70, 70, 255))));
}


void iAHistogramWidget::datatypehistograminitialize(iAPlotData::DataType* histlistptr, bool resetFunction,
	iAPlotData::DataType min, iAPlotData::DataType max, int bins, double space)
{
	m_data = iAHistogramData::Create(histlistptr, bins, space, min, max);
	if (resetFunction)
		functions[0]->reset();
}
