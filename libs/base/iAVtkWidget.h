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

#include "iAVtkVersion.h"

#include <vtkGenericOpenGLRenderWindow.h>

#if (VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(8, 2, 0))
	#include <QVTKOpenGLNativeWidget.h>
	using iAVtkWidget = QVTKOpenGLNativeWidget;
#else
	#include <QVTKOpenGLWidget.h>
	using iAVtkWidget = QVTKOpenGLWidget;
#endif

//! Unified interface to a Qt widget with VTK content, providing consistent usage for VTK versions 8 to 9.
class iAQVTKWidget: public iAVtkWidget
{
public:
	//! Creates the widget; makes sure its inner vtk render window is set, and sets an appropriate surface format
	iAQVTKWidget(QWidget* parent = nullptr): iAVtkWidget(parent)
	{	// before version 9, VTK did not set a default render window, let's do this...
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		SetRenderWindow(renWin);
#endif
		setFormat(iAVtkWidget::defaultFormat());
	}
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	// There also were no Qt-style methods to retrieve render window and interactor, let's provide them:
	vtkRenderWindow* renderWindow()
	{
		return GetRenderWindow();
	}
	QVTKInteractor* interactor()
	{
		return GetInteractor();
	}
#endif
};
