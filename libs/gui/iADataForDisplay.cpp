#include "iADataForDisplay.h"

#include "iAVolumeDataForDisplay.h"

#include "iADataSet.h"

std::shared_ptr<iADataForDisplay> createDataForDisplay(iADataSet* dataSet)
{
	auto volData = dynamic_cast<iAImageData*>(dataSet);
	if (volData)
	{
		return std::make_shared<iAVolumeDataForDisplay>(volData);
	}
	return std::shared_ptr<iADataForDisplay>();
}
