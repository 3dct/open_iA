#pragma once
#include "iAModuleInterface.h"
#include "dlg_TripleHistogramTF.h"

class iATripleHistogramTFModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void TestAction();
private:
	dlg_TripleHistogramTF * thtf;
};