/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "ui_ElementRenderer.h"
#include "iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QDockWidget, Ui_elementRenderer>   dlg_elemRendererContainer;

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iARenderer;
class iAVolumeRenderer;
class iAVolumeSettings;

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPolyData;
class vtkTransform;

class dlg_elementRenderer : public dlg_elemRendererContainer
{
	Q_OBJECT
public:
	dlg_elementRenderer(QWidget *parent);
	void removeObserver();

	void SetDataToVisualize(vtkImageData * imgData, vtkPolyData * polyData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf);
	iARenderer * GetRenderer();
	void SetRefLibIndex(size_t index);
	size_t GetRefLibIndex();
	void ApplyVolumeSettings(iAVolumeSettings const & vs);

private:
	iARenderer * m_renderer;
	bool m_rendInitialized;
	vtkSmartPointer<vtkTransform> m_axesTransform;
	QSharedPointer<iAVolumeRenderer> m_volumeRenderer;
	vtkRenderer * m_observedRenderer;
	unsigned long m_tag;
	size_t m_indexInReferenceLib;
};
