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
