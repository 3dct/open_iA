// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	iAFrustumActor* fsFrustum;
	iAFrustumActor* vrFrustum;
	QTimer m_updateRenderer;
	bool m_updateRequired;

private slots:
	void info();
	void startXVRA();

};
