// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FeatureScout_export.h"

#include <QToolBar>

#include <memory>

class iAMainWindow;
class iAMdiChild;
class Ui_FeatureScoutToolBar;

//! Manages the FeatureScout toolbar.
class FeatureScout_API iAFeatureScoutToolbar : public QToolBar
{
	Q_OBJECT
public:
	//! add child for which a FeatureScout toolbar should be available.
	static void addForChild(iAMainWindow* mainWnd, iAMdiChild* child);

private slots:
	void childClosed();
	void childChanged();

private:
	Q_DISABLE_COPY_MOVE(iAFeatureScoutToolbar);
	explicit iAFeatureScoutToolbar(iAMainWindow* mainWnd);
	~iAFeatureScoutToolbar() override; // required for std::unique_ptr
	static iAFeatureScoutToolbar* tlbFeatureScout;
	std::unique_ptr<Ui_FeatureScoutToolBar> m_ui;
	iAMainWindow* m_mainWnd;
};
