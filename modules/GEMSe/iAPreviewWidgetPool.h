// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>

#include <QVector>
#include <QWidget>

class iAColorTheme;
class iAImagePreviewWidget;

class vtkCamera;

class iAPreviewWidgetPool
{
public:
	//! create a pool with the given amount of widgets
	iAPreviewWidgetPool(int maxWidgets, vtkCamera* camera, iASlicerMode slicerMode, int labelCount, iAColorTheme const * colorTheme);
	//! retrieve a widget from the pool
	iAImagePreviewWidget* getWidget(QWidget* parent, bool magicLens=false);
	//! return widget to the pool (because it is not used anymore)
	//! hides the widget
	void returnWidget(iAImagePreviewWidget*);
	//! set slicer mode for all widgets
	void setSlicerMode(iASlicerMode mode, int sliceNr, vtkCamera* camera);
	//! change the shown slice number
	void setSliceNumber(int sliceNr);
	//! update all views
	void updateViews();
	//! retrieve the number of currently available widgets
	qsizetype capacity();
	void setColorTheme(iAColorTheme const * colorTheme);
private:
	QVector<iAImagePreviewWidget*> m_pool;
	QVector<iAImagePreviewWidget*> m_visible;
	vtkCamera*   m_commonCamera;
	iASlicerMode m_slicerMode;
	int m_sliceNumber[SlicerCount];
	int m_labelCount;
	iAColorTheme const * m_colorTheme;
};
