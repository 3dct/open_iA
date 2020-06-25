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

#if (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= QT_VERSION_CHECK(5, 4, 0) )

#include <QOpenGLWidget>

typedef QOpenGLWidget iAQGLWidget;
typedef QSurfaceFormat iAQGLFormat;

#define GRAB_FRAMEBUFFER grabFramebuffer

#else

#define WIN32_LEAN_AND_MEAN		// apparently QGLWidget might include windows.h...
#define VC_EXTRALEAN
#define NOMINMAX

#include <QGLWidget>

typedef QGLWidget iAQGLWidget;
typedef QGLFormat iAQGLFormat;

#define GRAB_FRAMEBUFFER grabFrameBuffer

#endif