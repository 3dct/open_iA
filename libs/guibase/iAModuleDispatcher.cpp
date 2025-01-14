// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAModuleDispatcher.h"

#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAFilterRunnerRegistry.h"
#include "iAFilterRunnerGUI.h"
#include "iAFilterSelectionDlg.h"
#include "iAGUIModuleInterface.h"
#include "iALog.h"
#include "iAMainWindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMenu>

#ifdef _MSC_VER
#define CALLCONV __stdcall

// inspired by https://stackoverflow.com/a/17387176
QString GetLastErrorAsString()
{
	const DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
	{
		return QString();
	}
	wchar_t * messageBuffer = nullptr;
	const DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);
	if (size == 0)
	{
		return QString("Details on the error could not be determined.");
	}
	QString result(QString::fromWCharArray(messageBuffer).trimmed());
	LocalFree(messageBuffer);
	return result;
}

#else
#define CALLCONV
#include <dlfcn.h>
#endif

typedef iAModuleInterface*(CALLCONV *f_GetModuleInterface)();


iALoadedModule::iALoadedModule():
	handle(nullptr), moduleInterface(nullptr)
{}

iALoadedModule::iALoadedModule(QString const & n, MODULE_HANDLE h, iAModuleInterface* i) :
	name(n), handle(h), moduleInterface(i)
{}


iAModuleDispatcher::iAModuleDispatcher(iAMainWindow * mainWnd)
{
	m_mainWnd = mainWnd;
	m_rootPath = QCoreApplication::applicationDirPath();
#ifdef __APPLE__
	// on apple, the executable resides in a subfolder of the "bundle"
	m_rootPath = m_rootPath.left(m_rootPath.length() -
		// hack - subtract the subfolder
		QString("open_iA.app/Contents/MacOS").length());
#endif
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
	// TODO: check what happens if LoadModule works, but Initialize fails?
	initializeModuleInterface(m);
	m_loadedModules.push_back(iALoadedModule(fi.completeBaseName(), handle, m));
	return m;
}

void iAModuleDispatcher::InitializeModules()
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
		LOG(lvlError, msg);
	}
	if (!m_mainWnd)	// all non-GUI related stuff already done
	{
		return;
	}
	auto filterFactories = iAFilterRegistry::filterFactories();
	for (size_t i=0; i<filterFactories.size(); ++i)
	{
		auto filterFactory = filterFactories[i];
		auto filter = filterFactory();
		QMenu * filterMenu = m_mainWnd->filtersMenu();
		QStringList categories = filter->fullCategory().split("/", Qt::SkipEmptyParts);
		for (auto cat : categories)
		{
			assert(!cat.isEmpty());   // if (cat.isEmpty()) LOG(lvlDebug, QString("Filter %1: Invalid category %2 - empty section!").arg(filter->name()).arg(filter->fullCategory()));
			filterMenu = getOrAddSubMenu(filterMenu, cat);
		}
		QAction * filterAction = new QAction(tr(filter->name().toStdString().c_str()), m_mainWnd);
		addToMenuSorted(filterMenu, filterAction);
		if (filter->requiredImages() > 0 || filter->requiredMeshes() > 0)
		{
			m_mainWnd->makeActionChildDependent(filterAction);
		}
		filterAction->setData(static_cast<quint64>(i));
		connect(filterAction, &QAction::triggered, this, &iAModuleDispatcher::executeFilter);
	}
	// enable Tools and Filters only if any modules were loaded that put something into them:
	m_mainWnd->toolsMenu()->menuAction()->setVisible(m_mainWnd->toolsMenu()->actions().size() > 0);
	m_mainWnd->filtersMenu()->menuAction()->setVisible(m_mainWnd->filtersMenu()->actions().size() > 0);

	if (m_mainWnd->filtersMenu()->actions().size() > 0)
	{
		QMenu * filterMenu = m_mainWnd->filtersMenu();
		QAction * selectAndRunFilterAction = new QAction(tr("Select and Run Filter..."), m_mainWnd);
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
	auto runner = iAFilterRunnerRegistry::filterRunner(filterID);
	m_runningFilters.push_back(runner);
	connect(runner.get(), &iAFilterRunnerGUI::finished, this, &iAModuleDispatcher::removeFilter);
	runner->run(iAFilterRegistry::filterFactories()[filterID](), m_mainWnd);
}

void iAModuleDispatcher::removeFilter()
{
	auto filterRunner = qobject_cast<iAFilterRunnerGUI*>(sender());
	for (int i = 0; i < m_runningFilters.size(); ++i)
	{
		if (m_runningFilters[i].get() == filterRunner)
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
