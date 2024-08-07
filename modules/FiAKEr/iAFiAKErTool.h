// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <memory>

class iAFiAKErController;

class iAFiAKErTool : public iATool
{
public:
	iAFiAKErTool(iAMainWindow* mainWnd, iAMdiChild* child);
	virtual ~iAFiAKErTool();
	void loadState(QSettings& projectFile, QString const& fileName) override;
	void saveState(QSettings& projectFile, QString const& fileName) override;
	iAFiAKErController* controller();

private:
	iAFiAKErController* m_controller;
};
