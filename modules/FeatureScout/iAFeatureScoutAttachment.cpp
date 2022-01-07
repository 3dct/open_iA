/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iA3DObjectFactory.h>
#include <iACsvConfig.h>

#include <iAModality.h>
#include <iAModalityTransfer.h>
#include <iAMdiChild.h>

#include <vtkTable.h>

iAFeatureScoutAttachment::iAFeatureScoutAttachment(iAMainWindow* mainWnd, iAMdiChild * child) :
	iAModuleAttachmentToChild(mainWnd, child)
{
}

iAFeatureScoutAttachment::~iAFeatureScoutAttachment()
{
	delete m_featureScout;
}

void iAFeatureScoutAttachment::init(int filterID, QString const & fileName, vtkSmartPointer<vtkTable> csvtbl,
	int visType, QSharedPointer<QMap<uint, uint> > columnMapping, std::map<size_t,
	std::vector<iAVec3f> > & curvedFiberInfo, int cylinderQuality, size_t segmentSkip)
{
	auto objvis = create3DObjectVis(visType, csvtbl, columnMapping,
		QColor(dlg_FeatureScout::UnclassifiedColorName), curvedFiberInfo, cylinderQuality, segmentSkip,
		visType == iACsvConfig::UseVolume ? m_child->modality(0)->transfer()->colorTF() : nullptr,
		visType == iACsvConfig::UseVolume ? m_child->modality(0)->transfer()->opacityTF() : nullptr,
		visType == iACsvConfig::UseVolume ? m_child->modality(0)->image()->GetBounds() : nullptr);
	m_featureScout = new dlg_FeatureScout(
		m_child, static_cast<iAObjectType>(filterID),
		fileName, csvtbl, visType, columnMapping, objvis);
}
