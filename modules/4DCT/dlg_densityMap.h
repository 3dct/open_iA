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
 
#ifndef DLG_DENSITYMAP_H
#define DLG_DENSITYMAP_H
// Ui
#include "ui_iA4DCTDensityMap.h"
// iA
//#include "iAQTtoUIConnector.h"
// vtk
#include <vtkSmartPointer.h>
// std
#include <vector>
// Qt
#include <QDockWidget>

class vtkImageData;
class MainWindow;
class MdiChild;

//typedef iAQTtoUIConnector<QDockWidget, Ui_DensityMap> dlg_densityMapConnector;

class dlg_densityMap : public QDockWidget, public Ui::DensityMap
{
	Q_OBJECT

public:
	dlg_densityMap(MainWindow* mWindow, MdiChild* mdiChild);
	~dlg_densityMap();
	void InitChannel();

private:
	vtkSmartPointer<vtkImageData> densityMap;
	MainWindow* m_mainWindow;
	MdiChild* m_mdiChild;
	std::vector<MdiChild*> m_lMdiChilds;

protected slots:
	void loadImages();
	void calcDensityMap();
};

#endif // DLG_DENSITYMAP_H
