// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAQVTKWidget.h>

#include <QMouseEvent>

//! Qt+VTK widget which emits signals when button released.
//! Solution for a "non-bug" in VTK http://www.vtk.org/pipermail/vtkusers/2013-December/082291.html
//! which will not get fixed.
class iAFuzzyVTKWidget : public iAQVTKWidget
{
	Q_OBJECT
public:
	iAFuzzyVTKWidget(QWidget* parent = nullptr);
protected:
	void mouseReleaseEvent ( QMouseEvent * event ) override;
	void resizeEvent ( QResizeEvent * event ) override;
signals:
	void rightButtonReleasedSignal();
	void leftButtonReleasedSignal();
};
