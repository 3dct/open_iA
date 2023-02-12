// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

class iAThemeChangeNotifier : public QObject
{
	Q_OBJECT
public:
	iAThemeChangeNotifier();
	~iAThemeChangeNotifier();
	void stop();
	static iAThemeChangeNotifier* get();
	void emitThemeChanged(bool brightTheme);
	static bool isBrightTheme();
#ifdef _MSC_VER
	void* m_stopEvent;
#endif
	bool m_isBright;

signals:
	void themeChanged(bool brightTheme);
};
