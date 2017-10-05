/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "pch.h"
#include "iAModuleDispatcher.h"

#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAFilterRunner.h"
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
#elif defined(__APPLE__) && defined(__MACH__)
	nameFilter << "*.dylib";
#else
	nameFilter << "*.so";
#endif
	
	QFileInfoList list = root.entryInfoList(nameFilter, QDir::Files);
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
	auto filterFactories = iAFilterRegistry::FilterFactories();
	for (int i=0; i<filterFactories.size(); ++i)
	{
		auto filterFactory = filterFactories[i];
		auto filter = filterFactory->Create();
		filter->Name();
		QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
		QMenu * menuCategory = getMenuWithTitle(filtersMenu, filter->Category());
		// TODO: allow for subcategories (with slashes?)
		QAction * filterAction = new QAction(QApplication::translate("MainWindow", filter->Name().toStdString().c_str(), 0), m_mainWnd);
		AddActionToMenuAlphabeticallySorted(menuCategory, filterAction);
		filterAction->setData(i);
		connect(filterAction, SIGNAL(triggered()), this, SLOT(ExecuteFilter()));
	}
	// enable Tools and Filters only if any modules were loaded that put something into them:
	m_mainWnd->getToolsMenu()->menuAction()->setVisible(m_mainWnd->getToolsMenu()->actions().size() > 0);
	m_mainWnd->getFiltersMenu()->menuAction()->setVisible(m_mainWnd->getFiltersMenu()->actions().size() > 0);
}

void iAModuleDispatcher::ExecuteFilter()
{
	int filterID = qobject_cast<QAction *>(sender())->data().toInt();
	RunFilter(iAFilterRegistry::FilterFactories()[filterID]->Create(), m_mainWnd);
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


QMenu * iAModuleDispatcher::getMenuWithTitle(QMenu * parentMenu, QString const & title, bool isDisablable)
{
	QList<QMenu*> submenus = parentMenu->findChildren<QMenu*>();
	for (int i = 0; i < submenus.size(); ++i)
	{
		if (submenus.at(i)->title() == title)
			return  submenus.at(i);
	}
	QMenu * result = new QMenu(parentMenu);
	result->setTitle(title);
	AddActionToMenuAlphabeticallySorted(parentMenu, result->menuAction(), isDisablable);
	return result;
}


void  iAModuleDispatcher::AddActionToMenuAlphabeticallySorted(QMenu * menu, QAction * action, bool isDisablable)
{
	AddModuleAction(action, isDisablable);
	foreach(QAction * curAct, menu->actions())
	{
		if (curAct->text() > action->text())
		{
			menu->insertAction(curAct, action);
			return;
		}
	}
	menu->addAction(action);
}