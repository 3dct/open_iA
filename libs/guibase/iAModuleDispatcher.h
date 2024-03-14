// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h> // for HINSTANCE / LPCWSTR
#endif

class iAFilterRunnerGUI;
class iAMainWindow;

class QFileInfo;
class QMenu;

class iAModuleInterface;

#ifdef _WIN32
	#define MODULE_HANDLE HINSTANCE
#else
	#define MODULE_HANDLE void*
#endif

//! Data associated with a single loaded module.
struct iAguibase_API iALoadedModule
{
	iALoadedModule();
	iALoadedModule(QString const & n, MODULE_HANDLE h, iAModuleInterface* i);
	QString            name;
	MODULE_HANDLE      handle;
	iAModuleInterface* moduleInterface;
};

//! Responsible for managing (i.e. loading, initializing and properly shutting down) all modules existing in the plugin folder.
class iAguibase_API iAModuleDispatcher : public QObject
{
	Q_OBJECT
public:
	iAModuleDispatcher( iAMainWindow * mainWnd );
	iAModuleDispatcher(QString const & rootPath);
	~iAModuleDispatcher();
	void InitializeModules();
	void SaveModulesSettings() const;
	template <typename T> T* module();
private slots:
	void executeFilter();
	void removeFilter();
	void selectAndRunFilter();
private:
	iAMainWindow * m_mainWnd;
	QVector< iALoadedModule > m_loadedModules;
	//! list of running filters, required to keep track of used memory
	//! could probably replaced by just new iAFilterRunnerGUI... followed by finished->deleteLater connection
	QVector<std::shared_ptr<iAFilterRunnerGUI>> m_runningFilters;
	QString m_rootPath;
	iAModuleInterface* loadModuleAndInterface(QFileInfo fi, QStringList & errorMessages);
	void initializeModuleInterface(iAModuleInterface* m);
	void runFilter(int filterID);
};

template <typename T> T* iAModuleDispatcher::module()
{
	for (iALoadedModule m: m_loadedModules)
	{
		T* ptr = dynamic_cast<T*>(m.moduleInterface);
		if (ptr)
		{
			return ptr;
		}
	}
	return nullptr;
}
