// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "iAGUIModuleInterface.h"

class iAGeometricObjectsDialog;

class iAPolygonPrimitivesModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void addObject();
private:
	iAGeometricObjectsDialog* m_dlg;
};
