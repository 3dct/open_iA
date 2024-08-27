// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_ElementRenderer.h"

#include <iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <memory>

class iARenderer;
class iAVolumeRenderer;
class iATransferFunctionPtrs;

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

private:
	iARendererImpl * m_renderer;
	vtkSmartPointer<vtkTransform> m_axesTransform;
	std::shared_ptr<iATransferFunctionPtrs> m_transferFunction;
	std::shared_ptr<iAVolumeRenderer> m_volumeRenderer;
	size_t m_indexInReferenceLib;
};
