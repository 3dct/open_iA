/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAVRAttachment.h"

// FeatureScout - 3D cylinder visualization
#include "dlg_CSVInput.h"
#include "iA3DCylinderObjectVis.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

#include <dlg_commoninput.h>
#include <iAModality.h>
#include <iAModalityTransfer.h>
#include <iAVolumeRenderer.h>

#include <vtkNamedColors.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkOpenVRCamera.h>

#include <vtkFloatArray.h>
#include <vtkTable.h>

// must be after vtk includes, otherwise -> #error:  gl.h included before glew.h
#include <mdichild.h>
#include <mainwindow.h>

iAVRAttachment::iAVRAttachment( MainWindow * mainWnd, iAChildData childData )
	: iAModuleAttachmentToChild( mainWnd, childData ), 
	m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New()),
	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()),
	m_interactor(vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New()),
	m_camera(vtkSmartPointer<vtkOpenVRCamera>::New())
{
	MdiChild * mdiChild = m_childData.child;
	
	m_renderWindow->AddRenderer(m_renderer);

	m_interactor->SetRenderWindow(m_renderWindow);
	m_renderer->SetActiveCamera(m_camera);
	auto colors = vtkSmartPointer<vtkNamedColors>::New();
	m_renderer->SetBackground(colors->GetColor3d("ForestGreen").GetData());

	// Volume rendering doesn't seem to work at the moment:
	//m_volumeRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(mdiChild->GetModality(0)->GetTransfer().get(), mdiChild->GetModality(0)->GetImage()));
	//iAVolumeSettings volSet;
	//volSet.RenderMode = 2;
	//m_volumeRenderer->ApplySettings(volSet);

	//m_volumeRenderer->AddTo(m_renderer);
	//m_volumeRenderer->AddBoundingBoxTo(m_renderer);

	//m_renderer->ResetCamera();

	dlg_CSVInput dlg(false);
	if (dlg.exec() != QDialog::Accepted)
		return;
	iACsvConfig csvConfig = dlg.getConfig();
	if (csvConfig.visType == iACsvConfig::UseVolume)
		return;

	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
		return;

	m_objectTable = creator.getTable();
	m_cylinderVis.reset(new iA3DCylinderObjectVis(m_renderer.Get(), m_objectTable, io.getOutputMapping(), QColor(255, 0, 0), 12));
	m_cylinderVis->show();

	m_renderWindow->Render();
	m_interactor->Start();
}

