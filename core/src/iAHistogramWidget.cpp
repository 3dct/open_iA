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
#include "iAHistogramWidget.h"

#include "dlg_function.h"
#include "iAHistogramData.h"
#include "iAFunctionDrawers.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

iAHistogramWidget::iAHistogramWidget(QWidget *parent, MdiChild * mdiChild, vtkImageAccumulate* accumulate,
		vtkPiecewiseFunction* oTF, vtkColorTransferFunction* cTF, QString label, bool reset) 
	: iADiagramFctWidget(parent, mdiChild, label)
{
	SetTransferFunctions(cTF, oTF);
	m_data = QSharedPointer<iAHistogramData>(new iAHistogramData());
	initialize(accumulate, reset);
	AddPlot(QSharedPointer<iAAbstractDrawableFunction>(new iABarGraphDrawer(m_data, QColor(70, 70, 70, 255))));
}

iAHistogramWidget::iAHistogramWidget(QWidget *parent,
	MdiChild * mdiChild,
	vtkImageAccumulate* accumulate,
	vtkPiecewiseFunction* oTF,
	vtkColorTransferFunction* cTF,
	iAPlotData::DataType* histData,
	iAPlotData::DataType dataMin,
	iAPlotData::DataType dataMax,
	int bins,
	double space,
	QString label,
	bool reset)
	: iADiagramFctWidget(parent, mdiChild, label)
{
	SetTransferFunctions(cTF, oTF);
	m_data = QSharedPointer<iAHistogramData>(new iAHistogramData());
	datatypehistograminitialize(accumulate, histData, reset, dataMin, dataMax, bins, space);
	AddPlot(QSharedPointer<iAAbstractDrawableFunction>(new iABarGraphDrawer(m_data, QColor(70, 70, 70, 255))));
}

void iAHistogramWidget::initialize(vtkImageAccumulate* accumulate, bool reset)
{
	m_data->initialize(accumulate);
	reInitialize(reset);
}


void iAHistogramWidget::datatypehistograminitialize(vtkImageAccumulate* hData, iAPlotData::DataType* histlistptr, bool reset,
	iAPlotData::DataType min, iAPlotData::DataType max, int bins, double space)
{
	m_data->initialize(hData, histlistptr, bins, space, min, max);
	reInitialize(reset);
}


void iAHistogramWidget::reInitialize(bool resetFunction)
{
	//set attribute, so that the objects are deleted while
	//this widget is closed
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setFocusPolicy(Qt::WheelFocus);
	
	mode = NO_MODE;
	
	draw = false;
	contextMenuVisible = false;
	updateAutomatically = true;
	
	setNewSize();

	selectedFunction = 0;
	//reset transfer function
	if (resetFunction)
	{
		functions[0]->reset();
	}
}

void iAHistogramWidget::UpdateData()
{
	m_data->UpdateData();
	SetMaxYAxisValue(m_data->YBounds()[1]);
}
