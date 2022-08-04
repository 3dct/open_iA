#pragma once

#include "iAgui_export.h"

#include "iADataForDisplay.h"

class iAImageData;
class iAModalityTransfer;

class iAgui_API iAVolumeDataForDisplay : public iADataForDisplay
{
public:
	iAVolumeDataForDisplay(iAImageData* data);
	void show(iAMdiChild* child) override;
	void close() override;
	iAModalityTransfer* transfer();
private:
	std::shared_ptr<iAModalityTransfer> m_transfer;

};