#include "iAElastixRegistrationModuleInterface.h"

#include "iAElastixRegistration.h"
#include "iATsvToVolume.h"

#include "iAFilterRegistry.h"

void iAElastixRegistrationModuleInterface::Initialize()
{
	REGISTER_FILTER(iAElastixRegistration);
	REGISTER_FILTER(iATsvToVolume);
}