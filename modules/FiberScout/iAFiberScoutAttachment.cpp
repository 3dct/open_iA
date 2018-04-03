/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"
#include "iAFiberScoutAttachment.h"

#include "dlg_FiberScout.h"
#include "iABlobCluster.h"
#include "iAObjectAnalysisType.h"

#include "iARenderer.h"
#include "mainwindow.h"

#include <vtkOpenGLRenderer.h>

iAFiberScoutAttachment::iAFiberScoutAttachment(MainWindow* mainWnd, iAChildData childData) :
	iAModuleAttachmentToChild(mainWnd, childData)
{
	blobRen = vtkSmartPointer<vtkOpenGLRenderer>::New();
	blobVisEnabled = false;
}

iAFiberScoutAttachment::~iAFiberScoutAttachment()
{}

void iAFiberScoutAttachment::init(int filterID, vtkSmartPointer<vtkTable> csvtbl)
{
	imgFS = new dlg_FiberScout(m_childData.child, static_cast<iAObjectAnalysisType>(filterID), blobRen, csvtbl);
	connect(imgFS, SIGNAL(updateViews()), m_childData.child, SLOT(updateViews()));

	blobRen->SetLayer(1);
	blobRen->UseDepthPeelingOn();
	blobRen->SetMaximumNumberOfPeels(12);

	m_childData.child->getRenderer()->AddRenderer(blobRen);
	blobRen->SetActiveCamera(m_childData.child->getRenderer()->getCamera());
	connect(m_childData.child->getRenderer(), SIGNAL(onSetCamera()), this, SLOT(rendererSetCamera()));
}

void iAFiberScoutAttachment::disableBlobVisualization()
{
	// we can't disable blob vis if it is already disabled
	if (!blobVisEnabled) return;
	blobVisEnabled = false;

	while (!blobList.isEmpty())
		delete blobList.takeFirst();
}

void iAFiberScoutAttachment::enableBlobVisualization()
{
	// we can't initialize blob vis twice
	if (blobVisEnabled) return;
	blobVisEnabled = true;
	vtkSmartPointer<vtkImageData> imageData = m_childData.child->getImagePointer();
	double size[3] = {
		imageData->GetBounds()[1] - imageData->GetBounds()[0],
		imageData->GetBounds()[3] - imageData->GetBounds()[2],
		imageData->GetBounds()[5] - imageData->GetBounds()[4]
	};
}


void iAFiberScoutAttachment::rendererSetCamera()
{
	blobRen->SetActiveCamera(m_childData.child->getRenderer()->getCamera());
}

bool iAFiberScoutAttachment::FiberScout_Options(int idx)
{
	if (!imgFS)
		return false;
	return imgFS->changeFiberScout_Options(idx);
}
