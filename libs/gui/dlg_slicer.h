// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_slicer.h"

#include "iASlicerMode.h"

#include <QDockWidget>

class iASlicerImpl;

class dlg_slicer : public QDockWidget, public Ui_slicer
{
Q_OBJECT

public:
	dlg_slicer(iASlicerImpl* slicer);
	void showBorder(bool show);
private slots:
	void setSliceSpinBox(int s);
	void setSliceScrollBar(int s);
	void setSlabMode(bool slabMode);
	void updateSlabThickness(int thickness);
	void updateSlabCompositeMode(int compositeMode);
	void updateSliceControls(int minIdx, int maxIdx, int val);
private:
	static const int BorderWidth;
	static QColor slicerColor(iASlicerMode mode);
	iASlicerImpl* m_slicer;
};
