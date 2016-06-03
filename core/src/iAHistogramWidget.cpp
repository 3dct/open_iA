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
#include "iAHistogramWidget.h"

#include "dlg_function.h"
#include "iAHistogramData.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPainter>
#include <QToolTip>
#include <QXmlStreamWriter>

iAHistogramWidget::iAHistogramWidget(QWidget *parent, MdiChild * mdiChild, double* scalarRange, vtkImageAccumulate* accumulate,
		vtkPiecewiseFunction* oTF, vtkColorTransferFunction* cTF, QString label, bool reset) 
	: iADiagramFctWidget(parent, mdiChild, oTF, cTF, label)
{
	data = QSharedPointer<iAHistogramData>(new iAHistogramData());
	
	initialize(accumulate, scalarRange, reset);
}

void iAHistogramWidget::initialize(vtkImageAccumulate* accumulate, double* scalarRange, bool reset)
{
	data->initialize(accumulate, scalarRange);
	reInitialize(reset);
}


void iAHistogramWidget::datatypehistograminitialize(vtkImageAccumulate* hData, iAAbstractDiagramData::DataType* histlistptr, bool reset,
	iAAbstractDiagramData::DataType min, iAAbstractDiagramData::DataType max, int bins, double space)
{
	data->initialize(hData, histlistptr, bins, space, min, max);
	reInitialize(reset);
}


void iAHistogramWidget::reInitialize(bool resetFunction)
{
	//set attribut, so that the objects are deleted while
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


QSharedPointer<iAAbstractDiagramRangedData> iAHistogramWidget::GetData()
{
	return data;
}

QSharedPointer<iAAbstractDiagramRangedData> const iAHistogramWidget::GetData() const
{
	return data;
}


void iAHistogramWidget::drawHistogram()
{
	drawEverything();
}
