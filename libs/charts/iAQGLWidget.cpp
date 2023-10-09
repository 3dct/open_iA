// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQGLWidget.h"

QSurfaceFormat defaultQOpenGLWidgetFormat()
{
	QSurfaceFormat fmt;
	fmt.setVersion(3, 2);
	fmt.setRenderableType(QSurfaceFormat::OpenGL);
	fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	fmt.setProfile(QSurfaceFormat::CoreProfile);
#ifdef OPENGL_DEBUG
	fmt.setOption(QSurfaceFormat::DebugContext);
#endif
	fmt.setSamples(8);
	fmt.setRedBufferSize(8);
	fmt.setGreenBufferSize(8);
	fmt.setBlueBufferSize(8);
	fmt.setDepthBufferSize(8);
	fmt.setAlphaBufferSize(8);
	fmt.setStencilBufferSize(0);
	/*
	LOG(lvlInfo, QString("Default GL format: version: %1.%2;"
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
