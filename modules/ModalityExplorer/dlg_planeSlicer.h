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

#include <vtkSmartPointer.h>

#include "ui_planeSlicer.h"
#include "iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QDockWidget, Ui_PlaneSlicer> dlg_planeSlicerUI;

#include <QtGlobal>
#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
class QVTKOpenGLWidget;
#else
class QVTKWidget2;
#endif
class vtkCamera;
class vtkColorTransferFunction;
class vtkImageData;
class vtkImageSlice;
class vtkPlane;
class vtkOpenGLRenderer;

class dlg_planeSlicer : public dlg_planeSlicerUI
{
	Q_OBJECT
public:
	dlg_planeSlicer();
	int AddImage(vtkSmartPointer<vtkImageData> image, vtkSmartPointer<vtkColorTransferFunction> lut, double initialOpacity);
	void SetCuttingPlane(double pos[3], double n[3]);
	void SetOpacity(int imageIdx, double opacity);
private:
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
	QVTKOpenGLWidget* m_vtkWidget;
#else
	QVTKWidget2* m_vtkWidget;
#endif
	vtkSmartPointer<vtkOpenGLRenderer> m_renderer;
	vtkCamera* m_camera_ext;

	QVector<vtkSmartPointer<vtkImageSlice> > m_images;
};
