// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASystemThemeWatcher.h"

#include <iALog.h>

#include <memory>

#ifdef _MSC_VER
#include <windows.h>
#endif

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

bool iASystemThemeWatcher::isBrightTheme()
{
#ifdef _MSC_VER
	/*
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
	QSettings personalize(
		"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
	return (personalize.value("AppsUseLightTheme").toInt() == 1);
#else
	return true;
#endif
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
						bool newBright = iASystemThemeWatcher::isBrightTheme();
						if (result->m_isBright != newBright)
						{
							result->m_isBright = newBright;
							emit result->themeChanged(newBright);
						}
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