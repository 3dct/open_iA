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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAHistogramWidget.h"
#include "open_iA_Core_export.h"
#include "iAQTtoUIConnector.h"
#include "ui_Histogram.h"

#include <vtkImageData.h>

#include <QDockWidget>
#include <QString>

typedef iAQTtoUIConnector<QDockWidget, Ui_Histogram>   dlg_histogramContainer;

class open_iA_Core_API dlg_histogram : public dlg_histogramContainer
{
	Q_OBJECT
public:
	iAHistogramWidget *histogram;
public:
	dlg_histogram(QWidget *parent, vtkImageData* input, vtkImageAccumulate* histData, vtkPiecewiseFunction* oTF, vtkColorTransferFunction* cTF, QString label = "Greyvalue")
					: dlg_histogramContainer( parent )
	{
		histogram = new iAHistogramWidget(parent, (MdiChild*)parent, input->GetScalarRange(), histData, oTF, cTF, label);
		histogram->setObjectName(QString::fromUtf8("histogram"));
		this->setWidget(histogram);
	}
	~dlg_histogram() {}
};
