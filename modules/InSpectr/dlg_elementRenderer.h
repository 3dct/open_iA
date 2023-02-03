// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_ElementRenderer.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iARenderer;
class iAVolumeRenderer;
class iAVolumeSettings;

class iARendererImpl;

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkTransform;

typedef iAQTtoUIConnector<QDockWidget, Ui_elementRenderer>   dlg_elemRendererContainer;

class dlg_elementRenderer : public dlg_elemRendererContainer
{
	Q_OBJECT
public:
	dlg_elementRenderer(QWidget *parent);

	void SetDataToVisualize(vtkImageData * imgData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf);
	iARenderer * GetRenderer();
	void SetRefLibIndex(size_t index);
	size_t GetRefLibIndex();
	void ApplyVolumeSettings(iAVolumeSettings const & vs);

private:
	iARendererImpl * m_renderer;
	bool m_rendInitialized;
	vtkSmartPointer<vtkTransform> m_axesTransform;
	QSharedPointer<iAVolumeRenderer> m_volumeRenderer;
	size_t m_indexInReferenceLib;
};
