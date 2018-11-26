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
#pragma once

#include "ui_TFTable.h"
#include "qthelper/iAQTtoUIConnector.h"

#include <vtkSmartPointer.h>

class dlg_function;
class iADiagramFctWidget;

class vtkPiecewiseFunction;
class vtkColorTransferFunction;

typedef iAQTtoUIConnector<QDialog, Ui_TFTableWidget>  dlg_TFTableWidgetConnector;

class dlg_TFTable : public dlg_TFTableWidgetConnector
{
	Q_OBJECT

public:
	dlg_TFTable( iADiagramFctWidget * parent, dlg_function* func );

public slots:
	void changeColor();
	void addPoint();
	void removeSelectedPoint();
	void updateHistogram();
	void itemClicked( QTableWidgetItem * );
	void cellValueChanged( int, int );
	void updateTable();

private:
	void Init();
	bool isValueXValid(double xVal, int row = -1);

	vtkSmartPointer<vtkPiecewiseFunction> m_oTF;
	vtkSmartPointer<vtkColorTransferFunction> m_cTF;
	QColor m_newPointColor;
	double m_xRange[2];
	double m_oldItemValue;
	iADiagramFctWidget* m_parent;
};
