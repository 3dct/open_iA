#pragma once
#include "iAModuleInterface.h"

class iAGeometricObjectsDialog;

class iAPolygonPrimitivesModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void addPolygonObject();
private:
	iAGeometricObjectsDialog* m_dlg;
};
