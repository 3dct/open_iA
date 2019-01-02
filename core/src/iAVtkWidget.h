#pragma once

#include <vtkVersion.h>

#include <QtGlobal>

#if (VTK_MAJOR_VERSION > 8 || (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) && (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400) )
	#include <QVTKOpenGLNativeWidget.h>
	#include <vtkGenericOpenGLRenderWindow.h>
	typedef QVTKOpenGLNativeWidget iAVtkWidget;
	typedef QVTKOpenGLNativeWidget iAVtkOldWidget;
	#define CREATE_OLDVTKWIDGET(x) \
	{ \
		(x) = new QVTKOpenGLNativeWidget(); \
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); \
		(x)->SetRenderWindow(renWin); \
	}
#else
	#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION < 2 && (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400) )
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
