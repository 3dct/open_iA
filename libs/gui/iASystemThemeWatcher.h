// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

//! Notifies about changes between dark/bright mode in the system that the application runs on.
//! Currently only supports Windows (10/11), and selected Linux window managers (tested with XFCE)
class iASystemThemeWatcher: public QObject
{
	Q_OBJECT
public:
	iASystemThemeWatcher();
	~iASystemThemeWatcher();
	//! get the (singleton) object that triggers notifications on theme change
	static iASystemThemeWatcher* get();
	//! whether currently the system has bright mode
	static bool isBrightTheme();
	//! stop looking for theme changes
	static void stop();
	//! trigger a check for whether something as changed (required for linux, where a changeEvent on main window (type StyleChanged(/ThemeChanged?) should trigger this)
	void checkForChange();

signals:
	//! notification  triggered when the theme switches between bright and dark
	void themeChanged(bool brightTheme);

private:
#ifdef _MSC_VER
	void* m_stopEvent;
#endif
	bool m_isBright;
};
