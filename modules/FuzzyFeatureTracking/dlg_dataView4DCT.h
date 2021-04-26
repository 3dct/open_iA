/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "ui_DataView4DCT.h"

#include <iARendererViewSync.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

typedef iAQTtoUIConnector<QDockWidget, Ui_DataView4DCT>	dlg_dataView4DCTContainer;

class iAQVTKWidgetMouseReleaseWorkaround;

class iARendererImpl;

class iAVolumeRenderer;
class iAVolumeStack;
class iAMdiChild;

class vtkCamera;
class vtkPolyData;
class vtkTransform;

class dlg_dataView4DCT : public dlg_dataView4DCTContainer
{
	Q_OBJECT
public:
	dlg_dataView4DCT(QWidget *parent, iAVolumeStack* volumeStack);
	~dlg_dataView4DCT();
	void update();

private:
	iAVolumeStack* m_volumeStack;
	iAQVTKWidgetMouseReleaseWorkaround** m_vtkWidgets;
	iARendererImpl**  m_renderers;
	iAVolumeRenderer** m_volumeRenderer;
	vtkSmartPointer<vtkTransform> m_axesTransform;
	iAMdiChild* m_mdiChild;
	iARendererViewSync m_rendererManager;
};
