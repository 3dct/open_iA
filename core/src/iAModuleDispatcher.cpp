/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAModuleDispatcher.h"

#include "dlg_FilterSelection.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAFilterRunnerGUI.h"
#include "iALogger.h"
#include "iAModuleInterface.h"
#include "mainwindow.h"

#include <QDir>
#include <QFileInfo>

#ifdef _MSC_VER
#define CALLCONV __stdcall

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
QString GetLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return QString();
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	QString result(message.c_str());
	return result.trimmed();
}

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
	m_rootPath = QCoreApplication::applicationDirPath();
}

iAModuleDispatcher::iAModuleDispatcher(QString const & rootPath): m_mainWnd(nullptr)
{
	m_rootPath = rootPath;
}

void CloseLibrary(iALoadedModule & module)
{
#ifdef _MSC_VER
/*
	// would have to delete everything (e.g. also menu entries, where it is currently failing)
	// created by modules first!
	if (FreeLibrary(module.handle) != TRUE)
	{
		DEBUG_LOG(QString("Error while unloading library %1: %2").arg(module.name).arg(GetLastErrorAsString()));
	}
*/
#else
/*
	// for unknown reason, unloading modules causes a segmentation fault under Linux
	if (dlclose(module.handle) != 0)
	{
		DEBUG_LOG(QString("Error while unloading library %1: %2").arg(module.name).arg(dlerror()));
	}
*/
#endif
}

QFileInfoList GetLibraryList(QString const & rootPath)
{
	QDir root(rootPath + "/plugins");
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

MODULE_HANDLE LoadModule(QFileInfo fileInfo, iALogger* logger)
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
	if (!hGetProcIDDLL)
	{
		logger->Log(QString("Could not load plugin %1: %2").arg(fileInfo.fileName()).arg(GetLastErrorAsString()));
	}
	return hGetProcIDDLL;
#else
	auto handle = dlopen(fileInfo.absoluteFilePath().toStdString().c_str(), RTLD_NOW);
	if (!handle)
	{
		logger->Log(QString("Could not load plugin %1: %2").arg(fileInfo.fileName()).arg(dlerror()));
	}
	return handle;
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
	MODULE_HANDLE handle = LoadModule(fi, logger);
	if (!handle)
	{
		return nullptr;
	}
	iAModuleInterface * m = LoadModuleInterface(handle);
	if (!m)
	{
		logger->Log(QString("Could not locate the GetModuleInterface function in '%1'").arg(fi.absoluteFilePath()));
		return nullptr;
	}
	InitializeModuleInterface(m);
	m_loadedModules.push_back(iALoadedModule(fi.completeBaseName(), handle, m));
	return m;
}

void iAModuleDispatcher::InitializeModules(iALogger* logger)
{
	QFileInfoList fList = GetLibraryList(m_rootPath);
	QFileInfoList failed;
	bool someNewLoaded = true;
	do
	{
		for (QFileInfo fi : fList)
		{
			if (!LoadModuleAndInterface(fi, logger))
				failed.push_back(fi);
		}
		someNewLoaded = failed.size() < fList.size();
		fList = failed;
	} while (fList.size() != 0 && someNewLoaded);
	if (!m_mainWnd)	// all non-GUI related stuff already done
	{
		return;
	}
	auto filterFactories = iAFilterRegistry::FilterFactories();
	for (int i=0; i<filterFactories.size(); ++i)
	{
		auto filterFactory = filterFactories[i];
		auto filter = filterFactory->Create();
		QMenu * filterMenu = m_mainWnd->getFiltersMenu();
		QStringList categories = filter->FullCategory().split("/");
		for (auto cat : categories)
			if (!cat.isEmpty())
				filterMenu = getMenuWithTitle(filterMenu, cat);
		QAction * filterAction = new QAction(QApplication::translate("MainWindow", filter->Name().toStdString().c_str(), 0), m_mainWnd);
		AddActionToMenuAlphabeticallySorted(filterMenu, filterAction);
		filterAction->setData(i);
		connect(filterAction, SIGNAL(triggered()), this, SLOT(ExecuteFilter()));
	}
	// enable Tools and Filters only if any modules were loaded that put something into them:
	m_mainWnd->getToolsMenu()->menuAction()->setVisible(m_mainWnd->getToolsMenu()->actions().size() > 0);
	m_mainWnd->getFiltersMenu()->menuAction()->setVisible(m_mainWnd->getFiltersMenu()->actions().size() > 0);

	if (m_mainWnd->getFiltersMenu()->actions().size() > 0)
	{
		QMenu * filterMenu = m_mainWnd->getFiltersMenu();
		QAction * selectAndRunFilterAction = new QAction(QApplication::translate("MainWindow", "Select and Run Filter...", 0), m_mainWnd);
		AddModuleAction(selectAndRunFilterAction, true);
		filterMenu->insertAction(filterMenu->actions()[0], selectAndRunFilterAction);
		connect(selectAndRunFilterAction, SIGNAL(triggered()), this, SLOT(SelectAndRunFilter()));
	}
}

void iAModuleDispatcher::ExecuteFilter()
{
	int filterID = qobject_cast<QAction *>(sender())->data().toInt();
	RunFilter(filterID);
}

void iAModuleDispatcher::SelectAndRunFilter()
{
	dlg_FilterSelection filterSelection(m_mainWnd);
	if (filterSelection.exec() == QDialog::Accepted)
		RunFilter(iAFilterRegistry::FilterID(filterSelection.SelectedFilterName()));
}

void iAModuleDispatcher::RunFilter(int filterID)
{
	if (filterID == -1)
		return;
	auto runner = iAFilterRegistry::FilterRunner(filterID)->Create();
	m_runningFilters.push_back(runner);
	connect(runner.data(), SIGNAL(finished()), this, SLOT(RemoveFilter()));
	runner->Run(iAFilterRegistry::FilterFactories()[filterID]->Create(), m_mainWnd);
}

void iAModuleDispatcher::RemoveFilter()
{
	auto filterRunner = qobject_cast<iAFilterRunnerGUI*>(sender());
	for (int i = 0; i < m_runningFilters.size(); ++i)
	{
		if (m_runningFilters[i].data() == filterRunner)
		{
			m_runningFilters.remove(i);
			break;
		}
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
