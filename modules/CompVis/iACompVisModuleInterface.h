#pragma once
#include <iAGUIModuleInterface.h>

class iACompVisModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize();

private slots:
	void CompVis();

private:

};