#pragma once
#include "iAModuleInterface.h"

class iACompVisModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();

private slots:
	void CompVis();

private:

};