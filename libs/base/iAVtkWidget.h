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
	using iAVtkOldWidget = QVTKOpenGLNativeWidget;
#else
	#include <QVTKOpenGLWidget.h>
	using iAVtkWidget = QVTKOpenGLWidget;
	using iAVtkOldWidget = QVTKOpenGLWidget;
#endif
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	#define CREATE_OLDVTKWIDGET(x) \
	{ \
		(x) = new iAVtkWidget(); \
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); \
		(x)->SetRenderWindow(renWin); \
		(x)->setFormat(iAVtkWidget::defaultFormat()); \
	}
#else
	#define CREATE_OLDVTKWIDGET(x) \
	{ \
		(x) = new iAVtkWidget(); \
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); \
		(x)->setRenderWindow(renWin); \
		(x)->setFormat(iAVtkWidget::defaultFormat()); \
	}
#endif

class iAQVTKWidget: public iAVtkWidget
{
public:
	iAQVTKWidget(QWidget* parent = nullptr): iAVtkWidget(parent)
	{
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		SetRenderWindow(renWin);
#else
		setRenderWindow(renWin);
#endif
		setFormat(iAVtkWidget::defaultFormat());
	}
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
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
