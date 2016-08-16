/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAModuleDispatcher.h"

#include "iALogger.h"
#include "iAModuleInterface.h"
#include "mainwindow.h"

#include <QDir>
#include <QFileInfo>

#include <sstream>

#ifdef _MSC_VER
#define CALLCONV __stdcall
#else
#define CALLCONV
#include <dlfcn.h>
#endif

typedef iAModuleInterface*(CALLCONV *f_GetModuleInterface)();


iALoadedModule::iALoadedModule():
	handle(0), moduleInterface(0)
{}

iALoadedModule::iALoadedModule(QString const & n, MODULE_HANDLE h, iAModuleInterface* i) :
	name(n), handle(h), moduleInterface(i)
{}


iAModuleDispatcher::iAModuleDispatcher(MainWindow * mainWnd)
{
	m_mainWnd = mainWnd;
}

void CloseLibrary(iALoadedModule & module)
{
#ifdef _MSC_VER
/*
	// would have to delete everything (e.g. also menu entries, where it is currently failing)
	// created by modules first!
	if (FreeLibrary(module.handle) != TRUE)
	{
		DEBUG_LOG(QString("Error while unloading library %1: %2").arg(module.name).arg(GetLastError()));
	}
*/
#else
	if (dlclose(module.handle) != 0)
	{
		// log?
	}
#endif
}

QFileInfoList GetLibraryList()
{
	QDir root(QCoreApplication::applicationDirPath() + "/plugins");
	QStringList nameFilter;
#ifdef _MSC_VER
	nameFilter << "*.dll";
#else
	nameFilter << "*.so";
#endif
	return root.entryInfoList(nameFilter, QDir::Files);
}

MODULE_HANDLE LoadModule(QFileInfo fileInfo)
{
#ifdef _MSC_VER
	QString dllWinName(fileInfo.absoluteFilePath());
	dllWinName.replace("/", "\\");
	std::wstring stemp = dllWinName.toStdWString();
	LPCWSTR dllName = stemp.c_str();
	UINT prevErrorMode = GetErrorMode();
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);  // to suppress message box on error
	HINSTANCE hGetProcIDDLL = LoadLibrary(dllName);
	SetErrorMode(prevErrorMode);
	return hGetProcIDDLL;
#else
/*
 * 	mode:
 * 	RTLD_LAZY 1
 * 	RTLD_NOW  2
 * 	RTLD_GLOBAL 4
 */
	return dlopen(fileInfo.absoluteFilePath().toStdString().c_str(), RTLD_NOW);
#endif
}

iAModuleInterface* LoadModuleInterface(MODULE_HANDLE handle)
{
	QString funcName = "GetModuleInterface";
#ifdef _MSC_VER
	f_GetModuleInterface func = (f_GetModuleInterface)GetProcAddress(handle, funcName.toStdString().c_str());
#else
	f_GetModuleInterface func = (f_GetModuleInterface) dlsym(handle, funcName.toStdString().c_str());
#endif
	return func();
}

iAModuleDispatcher::~iAModuleDispatcher()
{
	for (int i = 0; i < m_loadedModules.size(); ++i)
	{
		delete m_loadedModules[i].moduleInterface;
		CloseLibrary(m_loadedModules[i]);
	}
	m_loadedModules.clear();
}

void iAModuleDispatcher::InitializeModuleInterface(iAModuleInterface* m)
{
	m->SetMainWindow(m_mainWnd);
	m->SetDispatcher(this);
	m->Initialize();
}

iAModuleInterface* iAModuleDispatcher::LoadModuleAndInterface(QFileInfo fi, iALogger* logger)
{
	MODULE_HANDLE handle = LoadModule(fi);
	if (!handle)
	{
		logger->log(QString("Could not load the dynamic library '%1'").arg(fi.absoluteFilePath()));
		return NULL;
	}
	iAModuleInterface * m = LoadModuleInterface(handle);
	if (!m) {
		logger->log(QString("Could not locate the GetModuleInterface function in '%1'").arg(fi.absoluteFilePath()));
		return NULL;
	}
	InitializeModuleInterface(m);
	m_loadedModules.push_back(iALoadedModule(fi.completeBaseName(), handle, m));
	return m;
}

void iAModuleDispatcher::InitializeModules(iALogger* logger)
{
	QFileInfoList fList = GetLibraryList();
	for (QFileInfo fi : fList)
	{
		LoadModuleAndInterface(fi, logger);
	}
}

void iAModuleDispatcher::SaveModulesSettings() const
{
	for(iALoadedModule m: m_loadedModules)
	{
		m.moduleInterface->SaveSettings();
	}
}

MainWindow * iAModuleDispatcher::GetMainWnd() const
{
	return m_mainWnd;
}

void iAModuleDispatcher::AddModuleAction( QAction * action, bool isDisablable )
{
	iAModuleAction moduleAction( action, isDisablable );
	m_moduleActions.push_back( moduleAction );
}

void iAModuleDispatcher::SetModuleActionsEnabled( bool isEnabled )
{
	for (int i = 0; i < m_moduleActions.size(); ++i)
		if( m_moduleActions[i].isDisablable )
		{
			m_moduleActions[i].action->setEnabled( isEnabled );
			QMenu * actMenu = m_moduleActions[i].action->menu();
			if (actMenu)
				actMenu->setEnabled(isEnabled);
			
		}
}


void iAModuleDispatcher::ChildCreated(MdiChild* child)
{
	// notify all modules that a new child was created:
	for (iALoadedModule m : m_loadedModules)
	{
		m.moduleInterface->ChildCreated(child);
	}
}
