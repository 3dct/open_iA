// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <iAGUIModuleInterface.h>

#include "iAColoredPolyObjectVis.h"

#include <QTimer>

class dlg_FeatureScout;
class iAFrustumActor;
class iAImNDTModuleInterface;
class iAObjectData;

class iAXVRAModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	iAXVRAModuleInterface();
	void Initialize() override;
private:
	std::shared_ptr<iAColoredPolyObjectVis> m_polyObject;
	std::shared_ptr<iAObjectsData> m_objData;
	dlg_FeatureScout* m_fsMain;
	iAFrustumActor* fsFrustum;
	iAFrustumActor* vrFrustum;
	QTimer m_updateRenderer;
	bool m_updateRequired;

private slots:
	void info();
	void startXVRA();

};
