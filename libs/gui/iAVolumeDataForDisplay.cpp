#include "iAVolumeDataForDisplay.h"

#include "iADataSet.h"

#include "iAModalityTransfer.h"

#include <vtkImageData.h>

iAVolumeDataForDisplay::iAVolumeDataForDisplay(iAImageData* data) :
	m_transfer(std::make_shared<iAModalityTransfer>(data->image()->GetScalarRange()))
{
}

void iAVolumeDataForDisplay::show(iAMdiChild* child)
{
	Q_UNUSED(child);
	// compute histogram, show
}

void iAVolumeDataForDisplay::close()
{
	// close / delete open widgets
}

iAModalityTransfer* iAVolumeDataForDisplay::transfer()
{
	return m_transfer.get();
}