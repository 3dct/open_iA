// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASystemThemeWatcher.h"

#include <iALog.h>

#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#else
#include <QPalette>
#include <QStyle>
#endif

#include <memory>

#if defined(_MSC_VER) && QT_VERSION < QT_VERSION_CHECK(6, 5, 0)

#include <QtConcurrent>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#endif

iASystemThemeWatcher::iASystemThemeWatcher():
	m_isBright(iASystemThemeWatcher::isBrightTheme())
{
#if defined(_MSC_VER) && QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
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
#if defined(_MSC_VER) && QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
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
						//LOG(lvlDebug, "Stopping listening to registry value change!");
						running = false;
					}
					else
					{
						LOG(lvlError, QString("Waiting error; result: %1; error: %2")
							.arg(waitResult).arg(GetLastError()));
					}
					auto lErrorCode = RegCloseKey(hKey);
					if (lErrorCode != ERROR_SUCCESS)
					{
						LOG(lvlError, QString("Error in RegCloseKey (%1)").arg(GetLastError()));
					}
					if (!CloseHandle(hEvent))
					{
						LOG(lvlError, "Error in CloseHandle.");
						return;
					}
				}
				//LOG(lvlDebug, "Shutting down registry key change listener!");
			}
		);
#endif
		return r;
	} () ;  // directly call lambda instead of assigning it; we assign the lambda return value!
	return tcn.get();
}

bool iASystemThemeWatcher::isBrightTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light;
#else
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
	#if (__APPLE__)
		// inspired from https://stackoverflow.com/a/63035856
		// in contrast to below, here even the standard palette seems to get overwritten, that's why setPalette is disabled (see mainwindow.cpp)
		auto bg = qApp->palette().color(QPalette::Active, QPalette::Window);
		constexpr int OSX_LIGHT_MODE = 236;   //constexpr int OSX_DARK_MODE  = 50;
		//LOG(lvlDebug, QString("iASystemThemeWatcher: lightness: %1").arg(bg.lightness()));
		auto bright = (bg.lightness() == OSX_LIGHT_MODE);
	#else
		// inspired from comment on https://stackoverflow.com/a/69705673
		// we need to get style's standard palette here because the application palette is overwritten and does not automatically adapt to system one!
		auto const & p = qApp->style()->standardPalette();
		auto textColor = p.color(QPalette::WindowText);
		auto windowColor = p.color(QPalette::Window);
		auto bright = textColor.value() < windowColor.value();
		//LOG(lvlDebug, QString("iASystemThemeWatcher: isBrightTheme: textColor: %1; windowColor: %2; isBright: %3").arg(textColor.name()).arg(windowColor.name()).arg(bright));
	#endif
		return bright;
	#endif
#endif
}

void iASystemThemeWatcher::stop()
{
#if defined(_MSC_VER) && QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
	//LOG(lvlDebug, "iASystemThemeWatcher: Stopping!");
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
