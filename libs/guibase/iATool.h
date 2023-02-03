// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

class iAMainWindow;
class iAMdiChild;

class QSettings;
class QString;

class iAguibase_API iATool
{
public:
	//! implementation (empty) in iAToolRegistry.cpp
	iATool(iAMainWindow* mainWnd, iAMdiChild* child);
	//! virtual destructor, to enable proper destruction in derived classes and to avoid warnings
	virtual ~iATool();
	//! load the state of the tool from the  given settings
	virtual void loadState(QSettings & projectFile, QString const & fileName); // TODO: replace QSettings with QVariantMap? or at least make const (can't currently because of beginGroup/endGroup
	//! save the current state of the tool, so that the current window can be restored from the stored data via the loadState method
	virtual void saveState(QSettings & projectFile, QString const & fileName);
	//TODO NEWIO: maybe introduce the following functionality/flag:
	//! indicate whether this tool should be loaded only once the rendering of datasets has been fully initialized (true) or if it can be loaded once the child window is created (false)
	// virtual bool waitForRendering() const;
protected:
	iAMdiChild* m_child;
	iAMainWindow* m_mainWindow;
};
