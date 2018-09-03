#pragma once
#include "iAModuleInterface.h"
class dlg_TripleHistogramTF;

class iATripleHistogramTFModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void MenuItemSelected();
private:
	dlg_TripleHistogramTF * thtf;
};