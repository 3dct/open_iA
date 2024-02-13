// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <iAGUIModuleInterface.h>

#include <memory>

class iAFeatureScoutTool;
class iAFrustumActor;

class QAction;

class iAXVRAModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	iAXVRAModuleInterface();
	void Initialize() override;
private:
	QAction* m_actionXVRAStart;
	iAFrustumActor* fsFrustum;
	iAFrustumActor* vrFrustum;

private slots:
	void info();
	void startXVRA();

};
