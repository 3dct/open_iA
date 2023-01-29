// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAqthelper_export.h"

#include <QDockWidget>

class QString;

//! Show any arbitrary widget inside of a QDockWidget.
class iAqthelper_API iADockWidgetWrapper: public QDockWidget
{
public:
	iADockWidgetWrapper(QWidget* widget, QString const & windowTitle, QString const & objectName);
	void toggleTitleBar();
	bool isTitleBarVisible() const;
private:
	QWidget* m_titleBar;
};
