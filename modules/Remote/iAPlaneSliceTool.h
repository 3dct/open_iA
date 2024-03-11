// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <vtkSmartPointer.h>

class vtkPlaneWidget;
class vtkImageResliceMapper;
class vtkImageSlice;

class iADockWidgetWrapper;
class iAMainWindow;
class iAMdiChild;
class iAQVTKWidget;

class iAPlaneSliceTool : public iATool
{
public:
	static const QString Name;
	iAPlaneSliceTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAPlaneSliceTool();
private:
	iAQVTKWidget* m_sliceWidget;
	iADockWidgetWrapper* m_dw;
	vtkSmartPointer<vtkPlaneWidget> m_planeWidget;
	vtkSmartPointer<vtkImageResliceMapper> m_reslicer;
	vtkSmartPointer<vtkImageSlice> m_imageSlice;
};
