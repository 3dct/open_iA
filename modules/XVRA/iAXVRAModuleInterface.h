#pragma once
#include <iAGUIModuleInterface.h>

#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include <QSharedPointer>

// FeatureScout
#include "iA3DColoredPolyObjectVis.h"

class dlg_FeatureScout;
class iAImNDTModuleInterface;
class iAFrustumActor;

class iAXVRAModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private:
	QSharedPointer<iA3DColoredPolyObjectVis> m_polyObject;
	dlg_FeatureScout* m_fsMain;
	iAImNDTModuleInterface* m_vrMain;
	iAFrustumActor* fsFrustum;
	iAFrustumActor* vrFrustum;
	QAction* m_actionXVRA_ARView;
private slots:
	void info();
	void startXVRA();
	void startARView();
};