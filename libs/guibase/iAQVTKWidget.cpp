// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQVTKWidget.h"

#include <vtkRenderWindow.h>

#include <QEvent>

iAQVTKWidget::iAQVTKWidget(QWidget* parent) : iAVtkWidget(parent)
{
	setFormat(iAVtkWidget::defaultFormat());
}

void iAQVTKWidget::updateAll()
{
	renderWindow()->Render();
	update();
}

bool iAQVTKWidget::event(QEvent* evt)
{
	if (evt->type() == QEvent::HoverLeave)
	{   // QVTKOpengLNativeWidget triggers (up to 4) rendering on leave; we don't want that, try to work around:
		evt->accept();
		return true;
	}
	return iAVtkWidget::event(evt);
}
