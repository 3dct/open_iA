#include "iAElastixRegistrationModuleInterface.h"

#include "iAElastixRegistration.h"

#include "iAFilterRegistry.h"

void iAElastixRegistrationModuleInterface::Initialize()
{
	REGISTER_FILTER(iAElastixRegistration);
}