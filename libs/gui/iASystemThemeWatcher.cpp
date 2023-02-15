// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASystemThemeWatcher.h"

#include <iALog.h>

#include <memory>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QtConcurrent>

iASystemThemeWatcher::iASystemThemeWatcher():
	m_isBright(iASystemThemeWatcher::isBrightTheme())
{
#ifdef _MSC_VER
	m_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_stopEvent == NULL)
	{
		LOG(lvlError, QString("Error in CreateEvent (%1)!").arg(GetLastError()));
	}
#endif
}

iASystemThemeWatcher::~iASystemThemeWatcher()
{
}

iASystemThemeWatcher* iASystemThemeWatcher::get()
{
	static auto tcn = []()
	{
		auto r = std::make_unique<iASystemThemeWatcher>();
#ifdef _MSC_VER
		auto result = r.get();
		auto future = QtConcurrent::run([result]
			{
				bool running = true;
				while (running)
				{
					HKEY hKey;
					auto ThemeRegKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
					auto lResult = RegOpenKeyEx(HKEY_CURRENT_USER, ThemeRegKey, 0, KEY_READ, &hKey);
					if (lResult != ERROR_SUCCESS)
					{
						if (lResult == ERROR_FILE_NOT_FOUND)
						{
							LOG(lvlError, QString("Key not found!") /*.arg(ThemeRegKey - wstr)*/);
							return;
						}
						else
						{
							LOG(lvlError, "Error opening key!");
							return;
						}
					}
					HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					if (hEvent == NULL)
					{
						LOG(lvlError, QString("Error in CreateEvent (%1)!").arg(GetLastError()));
						return;
					}
					RegNotifyChangeKeyValue(hKey, false, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
					HANDLE handles[2] = { hEvent, result->m_stopEvent };
					auto waitResult = WaitForMultipleObjects(2, handles, false, INFINITE);
					if (waitResult == WAIT_OBJECT_0)
					{
						result->checkForChange();
					}
					else if (waitResult == WAIT_OBJECT_0 + 1)
					{
						LOG(lvlInfo, "Stopping listening to registry value change!");
						running = false;
					}
					else
					{
						LOG(lvlInfo, QString("Waiting error; result: %1; error: %2")
							.arg(waitResult).arg(GetLastError()));
					}
					auto lErrorCode = RegCloseKey(hKey);
					if (lErrorCode != ERROR_SUCCESS)
					{
						LOG(lvlInfo, QString("Error in RegCloseKey (%1)").arg(GetLastError()));
					}
					if (!CloseHandle(hEvent))
					{
						LOG(lvlInfo, "Error in CloseHandle.");
						return;
					}
				}
				LOG(lvlInfo, "Shutting down registry key change listener!");
			}
		);
#endif
		return r;
	} () ;  // directly call lambda instead of assigning it!
	return tcn.get();
}

bool iASystemThemeWatcher::isBrightTheme()
{
#ifdef _MSC_VER
	/*
	// not sure why but this code fails with error code 2:
	auto ValueKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize\\AppsUseLightTheme");
	DWORD value;
	DWORD size = sizeof(DWORD);
	auto readResult = RegQueryValueEx(HKEY_CURRENT_USER, ValueKey, NULL, NULL, reinterpret_cast<BYTE*>(&value), &size);
	if (readResult != ERROR_SUCCESS)
	{
		LOG(lvlError, QString("    Error reading value; error: %1!").arg(readResult));
		return true;
	}
	else
	{
		return (value == 1);
	}
	*/
	// this check is also in Qt, at plugins\platforms\windows\qwindowstheme.h|cpp -> queryDarkMode
	// on a quick glance this is not easy to access though, and not generically!
	QSettings personalize(
		"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
	return (personalize.value("AppsUseLightTheme").toInt() == 1);
#else
	// inspired from comment on https://stackoverflow.com/a/69705673
	// we need to get style's standard palette here because the application palette is overwritten and does not automatically adapt to system one!
	auto const & p = qApp->style()->standardPalette();
	auto textColor = p.color(QPalette::WindowText);
	auto windowColor = p.color(QPalette::Window);
	auto bright = textColor.value() < windowColor.value();
	LOG(lvlInfo, QString("isBrightTheme: textColor: %1; windowColor: %2; isBright: %3").arg(textColor.name()).arg(windowColor.name()).arg(bright));
	return bright;
#endif
}

void iASystemThemeWatcher::stop()
{
#ifdef _MSC_VER
	LOG(lvlInfo, "Stopping!");
	if (!SetEvent(get()->m_stopEvent))
	{
		LOG(lvlError, QString("Call to SetEvent failed: %1!").arg(GetLastError()));
	}
#endif
}

void iASystemThemeWatcher::checkForChange()
{
	bool newBright = iASystemThemeWatcher::isBrightTheme();
	if (m_isBright != newBright)
	{
		m_isBright = newBright;
		emit themeChanged(newBright);
	}
}
