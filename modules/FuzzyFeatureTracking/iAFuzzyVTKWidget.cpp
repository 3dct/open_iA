// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFuzzyVTKWidget.h"

iAFuzzyVTKWidget::iAFuzzyVTKWidget(QWidget* parent): iAQVTKWidget(parent)
{
}

void iAFuzzyVTKWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (Qt::RightButton == event->button())
	{
		emit rightButtonReleasedSignal();
	}
	else if (Qt::LeftButton == event->button())
	{
		emit leftButtonReleasedSignal();
	}
	iAQVTKWidget::mouseReleaseEvent(event);
}

void iAFuzzyVTKWidget::resizeEvent(QResizeEvent* event)
{
	repaint();//less flickering, but resize is less responsive
	iAQVTKWidget::resizeEvent(event);
}
