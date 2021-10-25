#pragma once
#include <iAGUIModuleInterface.h>

#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include <QSharedPointer>

// FeatureScout
#include "iA3DColoredPolyObjectVis.h"

class dlg_FeatureScout;
class iAVRMain;
class iAVREnvironment;
class iAVRInteractorStyle;

class iAXVRAModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private:
	QSharedPointer<iAVREnvironment> m_vrEnv;
	QSharedPointer<iA3DColoredPolyObjectVis> m_polyObject;
	dlg_FeatureScout* m_fsMain;
	iAVRMain* m_vrMain;
	vtkSmartPointer<iAVRInteractorStyle> m_style;
	vtkSmartPointer<vtkTable> m_objectTable;
private slots:
	void info();
	void startXVRA();
};