// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

class iASystemThemeWatcher: public QObject
{
	Q_OBJECT
public:
	iASystemThemeWatcher();
	~iASystemThemeWatcher();
	static iASystemThemeWatcher* get();
	static bool isBrightTheme();
	static void stop();

signals:
	void themeChanged(bool brightTheme);

private:
#ifdef _MSC_VER
	void* m_stopEvent;
#endif
	bool m_isBright;
};
