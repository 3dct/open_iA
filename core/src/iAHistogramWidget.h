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
#pragma once

#include "iADiagramFctWidget.h"
#include "open_iA_Core_export.h"

#include <QSharedPointer>

class vtkImageData;
class vtkImageAccumulate;

class iAHistogramData;


class open_iA_Core_API iAHistogramWidget : public iADiagramFctWidget
{
	Q_OBJECT

public:
	iAHistogramWidget(QWidget *parent,
		MdiChild * mdiChild,
		vtkImageAccumulate* histData,
		vtkPiecewiseFunction* oTF,
		vtkColorTransferFunction* cTF,
		QString label = "Greyvalue",
		bool reset = true);
	iAHistogramWidget(QWidget *parent,
		MdiChild * mdiChild,
		vtkImageAccumulate* accumulate,
		vtkPiecewiseFunction* oTF,
		vtkColorTransferFunction* cTF,
		iAPlotData::DataType* histData,
		iAPlotData::DataType min,
		iAPlotData::DataType max,
		int bins,
		double space,
		QString label,
		bool reset = true);
	void initialize(vtkImageAccumulate* histData, bool reset);
	void datatypehistograminitialize(vtkImageAccumulate* hData, iAPlotData::DataType* histData, bool reset,
		iAPlotData::DataType min, iAPlotData::DataType max, int bins, double space);
	void UpdateData();
private:
	void reInitialize(bool resetFunction);
	QSharedPointer<iAHistogramData> m_data;
};
