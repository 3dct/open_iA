#pragma once
#include <iAGUIModuleInterface.h>

class iAXVRAModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private slots:
	void testAction();
};