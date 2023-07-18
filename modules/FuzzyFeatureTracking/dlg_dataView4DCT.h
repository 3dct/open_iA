// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iARendererViewSync.h>

#include <vtkSmartPointer.h>

#include <QWidget>

class iAFuzzyVTKWidget;

class iARendererImpl;

class iAMdiChild;
class iAVolumeRenderer;
class iAVolumeViewer;

class vtkCamera;
class vtkPolyData;
class vtkTransform;

class dlg_dataView4DCT : public QWidget
{
	Q_OBJECT
public:
	dlg_dataView4DCT(QWidget* parent, std::vector<iAVolumeViewer*> const& volumeViewers);
	~dlg_dataView4DCT();
	void update();

private:
	std::vector<iAVolumeViewer*> m_volumeViewers;
	iAFuzzyVTKWidget** m_vtkWidgets;
	iARendererImpl**  m_renderers;
	iAVolumeRenderer** m_volumeRenderer;
	iAMdiChild* m_mdiChild;
	iARendererViewSync m_rendererManager;
};
