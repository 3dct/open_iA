/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <vtkVersion.h>

#include <QtGlobal>

#if (VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(8, 2, 0) && (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)) )
	#include <QVTKOpenGLNativeWidget.h>
	#include <vtkGenericOpenGLRenderWindow.h>
	typedef QVTKOpenGLNativeWidget iAVtkWidget;
	typedef QVTKOpenGLNativeWidget iAVtkOldWidget;
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	#define CREATE_OLDVTKWIDGET(x) \
	{ \
		(x) = new QVTKOpenGLNativeWidget(); \
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); \
		(x)->SetRenderWindow(renWin); \
	}
#else
	#define CREATE_OLDVTKWIDGET(x) \
	{ \
		(x) = new QVTKOpenGLNativeWidget(); \
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); \
		(x)->setRenderWindow(renWin); \
	}
#endif
#else
	#if (VTK_VERSION_NUMBER < VTK_VERSION_CHECK(8, 2, 0) && (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)) )
		#include <QVTKOpenGLWidget.h>
		#include <vtkGenericOpenGLRenderWindow.h>
		typedef QVTKOpenGLWidget iAVtkWidget;
		typedef QVTKOpenGLWidget iAVtkOldWidget;
		#define CREATE_OLDVTKWIDGET(x) \
		{ \
			(x) = new QVTKOpenGLWidget(); \
			auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); \
			(x)->SetRenderWindow(renWin); \
		}
	#else
		#include <QVTKWidget.h>
		#include <QVTKWidget2.h>
		#include <vtkRenderWindow.h>
		typedef QVTKWidget2 iAVtkWidget;
		typedef QVTKWidget iAVtkOldWidget;
		#define CREATE_OLDVTKWIDGET(x) (x) = new QVTKWidget();
	#endif
#endif
