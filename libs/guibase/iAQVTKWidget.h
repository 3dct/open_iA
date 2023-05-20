// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QVTKOpenGLNativeWidget.h>

using iAVtkWidget = QVTKOpenGLNativeWidget;

//! Unified interface to a Qt widget with VTK content, providing consistent usage for VTK versions 8 to 9.
class iAguibase_API iAQVTKWidget: public iAVtkWidget
{
public:
	//! Creates the widget; makes sure its inner vtk render window is set, and sets an appropriate surface format
	iAQVTKWidget(QWidget* parent = nullptr);
	void updateAll();
	bool event(QEvent* evt) override;
};
