// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QDockWidget>

class QString;

class iAQDockTitleWidget;

//! Wrapper to show an arbitrary widget inside of a QDockWidget.
//! Also adds a info button to the title bar which leads to a configurable info link.
class iAguibase_API iADockWidgetWrapper: public QDockWidget
{
public:
	iADockWidgetWrapper(QWidget* widget, QString const& windowTitle, QString const& objectName, QString const& infoLink = QString());
	void toggleTitleBar();
	bool isTitleBarVisible() const;
private:
	iAQDockTitleWidget* m_titleWidget;
};
