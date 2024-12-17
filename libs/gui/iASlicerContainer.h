// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_slicer.h"

#include "iASlicerMode.h"

#include <QWidget>

class iASlicerImpl;

//! Container for a slicer with controls for changing slice number, slab mode etc...
class iASlicerContainer : public QWidget, public Ui_slicer
{
Q_OBJECT

public:
	iASlicerContainer(iASlicerImpl* slicer);
	void showBorder(bool show);
	void showTitle(bool show);
	bool isTitleShown() const;
private slots:
	void setSliceSpinBox(int s);
	void setSliceScrollBar(int s);
	void setSlabMode(bool slabMode);
	void updateSlabThickness(int thickness);
	void updateSlabCompositeMode(int compositeMode);
	void updateSliceControls(int minIdx, int maxIdx, int val);
private:
	static const int BorderWidth;
	iASlicerImpl* m_slicer;
	bool m_titleVisible;
};
