/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QSharedPointer>

#include <vector>

class iAModalityList;
class iAQSplom;
class QTableWidget;

class vtkColorTransferFunction;
class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;

class dlg_modalitySPLOM: public QDockWidget
{
	Q_OBJECT
public:
	dlg_modalitySPLOM();
	~dlg_modalitySPLOM();
	void SetData(QSharedPointer<iAModalityList> img);
private slots:
	void SplomSelection(std::vector<size_t> const &);
private:
	iAQSplom* m_splom;
	QTableWidget* m_voxelData;
	vtkSmartPointer<vtkLookupTable> m_lut;
	int m_extent[6];
	double m_spacing[3];
	double m_origin[3];
	vtkSmartPointer<vtkColorTransferFunction> m_selection_ctf;
	vtkSmartPointer<vtkPiecewiseFunction> m_selection_otf;
	bool m_selected;
};
