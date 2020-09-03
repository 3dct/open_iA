/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAQGLWidget.h"

iAQGLFormat defaultOpenGLFormat()
{
	iAQGLFormat fmt;
#if (defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= QT_VERSION_CHECK(5, 4, 0) )
	fmt.setVersion(3, 2);
	fmt.setRenderableType(QSurfaceFormat::OpenGL);
	fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
#else
	fmt.setVersion(1, 0);
	fmt.setDoubleBuffer(true);
#endif
	fmt.setProfile(iAQGLFormat::CoreProfile);
	fmt.setSamples(8);
	fmt.setStereo(true);
	fmt.setRedBufferSize(8);
	fmt.setGreenBufferSize(8);
	fmt.setBlueBufferSize(8);
	fmt.setDepthBufferSize(8);
	fmt.setAlphaBufferSize(8);
	fmt.setStencilBufferSize(0);
	/*
	DEBUG_LOG(QString("Default GL format: version: %1.%2;"
		" buffer sizes r: %3, g: %4, b: %5, a: %6, d: %7, s: %8; stereo: %9, samples: %10, hasAlpha: %11")
		.arg(fmt.majorVersion())
		.arg(fmt.minorVersion())
		.arg(fmt.redBufferSize())
		.arg(fmt.greenBufferSize())
		.arg(fmt.blueBufferSize())
		.arg(fmt.alphaBufferSize())
		.arg(fmt.depthBufferSize())
		.arg(fmt.stencilBufferSize())
		.arg(fmt.stereo())
		.arg(fmt.samples())
		.arg(fmt.hasAlpha())
	);
	*/
	return fmt;
}