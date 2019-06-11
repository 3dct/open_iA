#pragma once
#include "iAModuleInterface.h"

class iAdaptiveThresholdModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void determineThreshold();
};