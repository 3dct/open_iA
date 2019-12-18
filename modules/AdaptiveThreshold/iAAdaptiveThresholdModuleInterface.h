#pragma once
#include "iAModuleInterface.h"

class iAAdaptiveThresholdModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void determineThreshold();
};