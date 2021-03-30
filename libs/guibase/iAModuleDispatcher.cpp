/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iALog.h"
#include "iAFilter.h"
#include "iAFilterRunnerRegistry.h"
#include "iAFilterRunnerGUI.h"
#include "iAFilterSelectionDlg.h"
#include "iAGUIModuleInterface.h"
#include "iALogger.h"
#include "iAModuleInterface.h"
#include "iAMainWindow.h"

#include <QDir>
#include <QFileInfo>
#include <QMenu>

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
		nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);
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


iAModuleDispatcher::iAModuleDispatcher(iAMainWindow * mainWnd)
{
	m_mainWnd = mainWnd;
	m_rootPath = QCoreApplication::applicationDirPath();
}

iAModuleDispatcher::iAModuleDispatcher(QString const & rootPath): m_mainWnd(nullptr)
{
	m_rootPath = rootPath;
}

void CloseLibrary(iALoadedModule & /*module*/)
{
#ifdef _MSC_VER
/*
	// would have to delete everything (e.g. also menu entries, where it is currently failing)
	// created by modules first!
	if (FreeLibrary(module.handle) != TRUE)
	{
		LOG(lvlError, QString("Error while unloading library %1: %2").arg(module.name).arg(GetLastErrorAsString()));
	}
*/
#else
/*
	// for unknown reason, unloading modules causes a segmentation fault under Linux
	if (dlclose(module.handle) != 0)
	{
		LOG(lvlError, QString("Error while unloading library %1: %2").arg(module.name).arg(dlerror()));
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

MODULE_HANDLE LoadModule(QFileInfo fileInfo, QStringList & errorMessages)
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
		errorMessages << QString("Could not load plugin %1: %2").arg(fileInfo.fileName()).arg(GetLastErrorAsString());
	}
	return hGetProcIDDLL;
#else
	auto handle = dlopen(fileInfo.absoluteFilePath().toStdString().c_str(), RTLD_NOW);
	if (!handle)
	{
		errorMessages << QString("Could not load plugin %1: %2").arg(fileInfo.fileName()).arg(dlerror());
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

void iAModuleDispatcher::initializeModuleInterface(iAModuleInterface* m)
{
	if (dynamic_cast<iAGUIModuleInterface*>(m))
	{
		dynamic_cast<iAGUIModuleInterface*>(m)->SetMainWindow(m_mainWnd);
	}
	m->Initialize();
}

iAModuleInterface* iAModuleDispatcher::loadModuleAndInterface(QFileInfo fi, QStringList & errorMessages)
{
	MODULE_HANDLE handle = LoadModule(fi, errorMessages);
	if (!handle)
	{
		return nullptr;
	}
	iAModuleInterface * m = LoadModuleInterface(handle);
	if (!m)
	{
		errorMessages << QString("Could not locate the GetModuleInterface function in '%1'").arg(fi.absoluteFilePath());
		return nullptr;
	}
	initializeModuleInterface(m);
	m_loadedModules.push_back(iALoadedModule(fi.completeBaseName(), handle, m));
	return m;
}

void iAModuleDispatcher::InitializeModules(iALogger* logger)
{
	QFileInfoList fList = GetLibraryList(m_rootPath);
	QFileInfoList failed;
	bool someNewLoaded = true;
	QStringList loadErrorMessages;
	do
	{
		loadErrorMessages.clear();
		for (QFileInfo fi : fList)
		{
			if (!loadModuleAndInterface(fi, loadErrorMessages))
			{
				failed.push_back(fi);
			}
		}
		someNewLoaded = failed.size() < fList.size();
		fList = failed;
	} while (fList.size() != 0 && someNewLoaded);
	for (auto msg : loadErrorMessages)
	{
		logger->log(lvlError, msg);
	}
	if (!m_mainWnd)	// all non-GUI related stuff already done
	{
		return;
	}
	auto filterFactories = iAFilterRegistry::filterFactories();
	for (int i=0; i<filterFactories.size(); ++i)
	{
		auto filterFactory = filterFactories[i];
		auto filter = filterFactory->create();
		QMenu * filterMenu = m_mainWnd->filtersMenu();
		QStringList categories = filter->fullCategory().split("/");
		for (auto cat : categories)
		{
			if (!cat.isEmpty())
			{
				filterMenu = getOrAddSubMenu(filterMenu, cat);
			}
		}
		QAction * filterAction = new QAction(tr(filter->name().toStdString().c_str()), m_mainWnd);
		addToMenuSorted(filterMenu, filterAction);
		m_mainWnd->makeActionChildDependent(filterAction);
		filterAction->setData(i);
		connect(filterAction, &QAction::triggered, this, &iAModuleDispatcher::executeFilter);
	}
	// enable Tools and Filters only if any modules were loaded that put something into them:
	m_mainWnd->toolsMenu()->menuAction()->setVisible(m_mainWnd->toolsMenu()->actions().size() > 0);
	m_mainWnd->filtersMenu()->menuAction()->setVisible(m_mainWnd->filtersMenu()->actions().size() > 0);

	if (m_mainWnd->filtersMenu()->actions().size() > 0)
	{
		QMenu * filterMenu = m_mainWnd->filtersMenu();
		QAction * selectAndRunFilterAction = new QAction(tr("Select and Run Filter..."), m_mainWnd);
		m_mainWnd->makeActionChildDependent(selectAndRunFilterAction);
		filterMenu->insertAction(filterMenu->actions()[0], selectAndRunFilterAction);
		connect(selectAndRunFilterAction, &QAction::triggered, this, &iAModuleDispatcher::selectAndRunFilter);
	}
}

void iAModuleDispatcher::executeFilter()
{
	int filterID = qobject_cast<QAction *>(sender())->data().toInt();
	runFilter(filterID);
}

void iAModuleDispatcher::selectAndRunFilter()
{
	iAFilterSelectionDlg filterSelection(m_mainWnd);
	if (filterSelection.exec() == QDialog::Accepted)
	{
		runFilter(iAFilterRegistry::filterID(filterSelection.selectedFilterName()));
	}
}

void iAModuleDispatcher::runFilter(int filterID)
{
	if (filterID == -1)
	{
		return;
	}
	auto runner = iAFilterRunnerRegistry::filterRunner(filterID)->create();
	m_runningFilters.push_back(runner);
	connect(runner.data(), &iAFilterRunnerGUI::finished, this, &iAModuleDispatcher::removeFilter);
	runner->run(iAFilterRegistry::filterFactories()[filterID]->create(), m_mainWnd);
}

void iAModuleDispatcher::removeFilter()
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

iAMainWindow * iAModuleDispatcher::GetMainWnd() const
{
	return m_mainWnd;
}

void iAModuleDispatcher::ChildCreated(iAMdiChild* child)
{
	// notify all modules that a new child was created:
	for (iALoadedModule m : m_loadedModules)
	{
		if (dynamic_cast<iAGUIModuleInterface*>(m.moduleInterface))
		{
			dynamic_cast<iAGUIModuleInterface*>(m.moduleInterface)->ChildCreated(child);
		}
	}
}
