/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASliceRenderer.h"

#include "iAChannelData.h"
#include "iADataSet.h"
#include "iAMdiChild.h"
#include "iAModalityTransfer.h"
#include "iASlicerImpl.h"
#include "iAVolumeDataForDisplay.h"

void iASliceRenderer::setVisible(bool visible)
{
	Q_UNUSED(visible);
}

class iAVolumeSliceRenderer : public iASliceRenderer
{
public:
	iAVolumeSliceRenderer(iAImageData* imgData, iAModalityTransfer* transfer, std::array<iASlicerImpl*, 3> slicer, uint channelID):
		m_channelID(channelID),
		m_slicer(slicer)
	{
		for (int s = 0; s < 3; ++s)
		{
			slicer[s]->addChannel(m_channelID, iAChannelData(imgData->name(), imgData->image(), transfer->colorTF()), true);
			slicer[s]->resetCamera();
		}
	}
	void setVisible(bool visible) override
	{
		for (int s = 0; s < 3; ++s)
		{
			m_slicer[s]->enableChannel(m_channelID, visible);
		}
	}
private:
	uint m_channelID;
	std::array<iASlicerImpl*, 3> m_slicer;
};

std::shared_ptr<iASliceRenderer> createSliceRenderer(iADataSet* dataSet, iADataForDisplay* dataForDisplay, std::array<iASlicerImpl*, 3> slicer, iAMdiChild* child)
{
	auto imgData = dynamic_cast<iAImageData*>(dataSet);
	if (imgData)
	{
		auto channelID = child->createChannel();
		return std::make_shared<iAVolumeSliceRenderer>(imgData, dynamic_cast<iAVolumeDataForDisplay*>(dataForDisplay)->transfer(), slicer, channelID);
	}
	return std::shared_ptr<iASliceRenderer>();
}
