// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASignallingWidget.h"
#include "iAQVTKWidget.h"

class iAColoredWidget;

//! Keeps the aspect ratio of a contained iAQVTKWidget fixed
//! by placing two other resizable widgets around it as padding.
class iAFixedAspectWidget: public iASignallingWidget
{
	Q_OBJECT
public:
	iAFixedAspectWidget(double aspect=1.0, Qt::Alignment verticalAlign = Qt::AlignVCenter);
	iAQVTKWidget* vtkWidget();
	void setBGRole(QPalette::ColorRole role);
private:
	iAQVTKWidget* m_widget;
	iAColoredWidget* m_fill1, * m_fill2;
};
