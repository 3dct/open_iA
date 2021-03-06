/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"

#include <QObject>
#include <QString>
#include <QVector>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h> // for HINSTANCE / LPCWSTR
#endif

class iAFilterRunnerGUI;
class iALogger;
class MdiChild;
class MainWindow;

class QAction;
class QFileInfo;
class QMenu;

//! Data for menu actions created automatically for filters.
struct iAModuleAction
{
	iAModuleAction(): action(nullptr), isDisablable(true) {}
	iAModuleAction( QAction * _action, bool _isDisablable ) : action( _action ), isDisablable( _isDisablable ) {}

	QAction * action;
	bool isDisablable;
};

class iAModuleInterface;

#ifdef _WIN32
	#define MODULE_HANDLE HINSTANCE
#else
	#define MODULE_HANDLE void*
#endif

//! Data associated with a single loaded module.
struct open_iA_Core_API iALoadedModule
{
	iALoadedModule();
	iALoadedModule(QString const & n, MODULE_HANDLE h, iAModuleInterface* i);
	QString            name;
	MODULE_HANDLE      handle;
	iAModuleInterface* moduleInterface;
};

//! Responsible for managing (i.e. loading, initializing and properly shutting down) all modules existing in the plugin folder.
class iAModuleDispatcher: public QObject
{
	Q_OBJECT
public:
	iAModuleDispatcher( MainWindow * mainWnd );
	iAModuleDispatcher(QString const & rootPath);
	~iAModuleDispatcher();
	void InitializeModules(iALogger* logger);
	void SaveModulesSettings() const;
	MainWindow * GetMainWnd() const;
	void AddModuleAction(QAction * action, bool isDisablable);
	void SetModuleActionsEnabled( bool isEnabled );
	template <typename T> T* GetModule();
	void ChildCreated(MdiChild* child);
	QMenu * getMenuWithTitle(QMenu * parentMenu, QString const & title, bool isDisablable = true);
	void AddActionToMenuAlphabeticallySorted(QMenu * menu, QAction * action, bool isDisablable = true);
private slots:
	void executeFilter();
	void removeFilter();
	void selectAndRunFilter();
private:
	MainWindow * m_mainWnd;
	QVector < iAModuleAction > m_moduleActions;
	QVector < iALoadedModule > m_loadedModules;
	QVector< QSharedPointer<iAFilterRunnerGUI> > m_runningFilters;
	QString m_rootPath;
	iAModuleInterface* loadModuleAndInterface(QFileInfo fi, QStringList & errorMessages);
	void initializeModuleInterface(iAModuleInterface* m);
	void runFilter(int filterID);
};

template <typename T> T* iAModuleDispatcher::GetModule()
{
	for (iALoadedModule m: m_loadedModules)
	{
		T* ptr = dynamic_cast<T*>(m.moduleInterface);
		if (ptr)
		{
			return ptr;
		}
	}
	return 0;
}
