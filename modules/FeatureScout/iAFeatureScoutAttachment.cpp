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
#include "iAFeatureScoutAttachment.h"

#include "dlg_FeatureScout.h"
#include "iABlobCluster.h"
#include "iAFeatureScoutObjectType.h"

#include <iARenderer.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>

iAFeatureScoutAttachment::iAFeatureScoutAttachment(MainWindow* mainWnd, MdiChild * child) :
	iAModuleAttachmentToChild(mainWnd, child)
{
	blobVisEnabled = false;
}

iAFeatureScoutAttachment::~iAFeatureScoutAttachment()
{}

void iAFeatureScoutAttachment::init(int filterID, QString const & fileName, vtkSmartPointer<vtkTable> csvtbl, 
	int visType, QSharedPointer<QMap<uint, uint> > columnMapping)
{
	imgFS = new dlg_FeatureScout(m_child, static_cast<iAFeatureScoutObjectType>(filterID),
		fileName, m_child->renderer()->renderer(), csvtbl, visType, columnMapping);
}

void iAFeatureScoutAttachment::disableBlobVisualization()
{
	// we can't disable blob vis if it is already disabled
	if (!blobVisEnabled) return;
	blobVisEnabled = false;

	while (!blobList.isEmpty())
		delete blobList.takeFirst();
}

void iAFeatureScoutAttachment::enableBlobVisualization()
{
	// we can't initialize blob vis twice
	if (blobVisEnabled) return;
	blobVisEnabled = true;
	vtkSmartPointer<vtkImageData> imageData = m_child->imagePointer();
	double size[3] = {
		imageData->GetBounds()[1] - imageData->GetBounds()[0],
		imageData->GetBounds()[3] - imageData->GetBounds()[2],
		imageData->GetBounds()[5] - imageData->GetBounds()[4]
	};
}

void iAFeatureScoutAttachment::FeatureScout_Options(int idx)
{
	if (!imgFS)
		return;
	imgFS->changeFeatureScout_Options(idx);
}
