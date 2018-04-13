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

#include <vtkSmartPointer.h>

#include <QVector>
#include <QWidget>

class iAParamSpatialView;
class iAParamTableView;
class iAQSplom;

class vtkColorTransferFunction;
class vtkLookupTable;
class vtkPiecewiseFunction;

class QCheckBox;

class iAParamSPLOMView: public QWidget
{
	Q_OBJECT
public:
	iAParamSPLOMView(iAParamTableView* tableView, iAParamSpatialView* spatialView);
private slots:
	void SetLUTColumn(QString const & colName);
	void SplomSelection(QVector<unsigned int> *);
	void UpdateFeatVisibilty(int);
	void PointHovered(int);
private:
	iAParamSpatialView* m_spatialView;
	iAParamTableView* m_tableView;
	iAQSplom* m_splom;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkColorTransferFunction> m_selection_ctf;
	vtkSmartPointer<vtkPiecewiseFunction> m_selection_otf;
	QWidget* m_settings;
	QVector<QCheckBox*> m_featCB;
};
