#pragma once

#include <vtkVersion.h>

#include <QtGlobal>

#if (VTK_MAJOR_VERSION > 8 || (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) && (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400) )
	class QVTKOpenGLNativeWidget;
	typedef QVTKOpenGLNativeWidget iAVtkWidget;
	typedef QVTKOpenGLNativeWidget iAVtkOldWidget;
#else
	#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION < 2 && (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400) )
		class QVTKOpenGLWidget;
		typedef QVTKOpenGLWidget iAVtkWidget;
		typedef QVTKOpenGLWidget iAVtkOldWidget;
	#else
		class QVTKWidget;
		class QVKTWidget2;
		typedef QVTKWidget2 iAVtkWidget;
		typedef QVTKWidget iAVtkOldWidget;
	#endif
#endif
