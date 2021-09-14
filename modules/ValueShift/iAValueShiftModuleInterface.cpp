#include "iAValueShiftModuleInterface.h"

#include "iAValueShiftFilter.h"

#include "iAFilterRegistry.h"

void iAValueShiftModuleInterface::Initialize()
{
	REGISTER_FILTER(iAValueShiftFilter);
}