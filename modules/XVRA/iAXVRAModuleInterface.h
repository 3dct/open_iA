#pragma once
#include <iAGUIModuleInterface.h>

// FeatureScout
#include "iA3DColoredPolyObjectVis.h"

#include <QTimer>

class dlg_FeatureScout;
class iAImNDTModuleInterface;
class iAFrustumActor;

class iAXVRAModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	iAXVRAModuleInterface();
	void Initialize() override;
private:
	QSharedPointer<iA3DColoredPolyObjectVis> m_polyObject;
	dlg_FeatureScout* m_fsMain;
	iAImNDTModuleInterface* m_vrMain;
	iAFrustumActor* fsFrustum;
	iAFrustumActor* vrFrustum;
	QTimer m_updateRenderer;
	bool m_updateRequired;

private slots:
	void info();
	void startXVRA();

};