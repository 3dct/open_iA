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
#pragma once

#include <QVector>
#include <QString>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // for HINSTANCE / LPCWSTR
#endif

class iALogger;
class MdiChild;
class MainWindow;

class QAction;
class QFileInfo;

struct iAModuleAction
{
	iAModuleAction() {}
	iAModuleAction( QAction * _action, bool _isDisablable ) : action( _action ), isDisablable( _isDisablable ) {}

	QAction * action;
	bool isDisablable;
};

class iAModuleInterface;

#ifdef WIN32
	#define MODULE_HANDLE HINSTANCE
#else
	#define MODULE_HANDLE void*
#endif

struct iALoadedModule
{
	iALoadedModule();
	iALoadedModule(QString const & n, MODULE_HANDLE h, iAModuleInterface* i);
	QString            name;
	MODULE_HANDLE      handle;
	iAModuleInterface* moduleInterface;
};

class iAModuleDispatcher
{
public:
	iAModuleDispatcher( MainWindow * mainWnd );
	~iAModuleDispatcher();
	void InitializeModules(iALogger* logger);
	void SaveModulesSettings() const;
	MainWindow * GetMainWnd() const;
	void AddModuleAction(QAction * action, bool isDisablable);
	void SetModuleActionsEnabled( bool isEnabled );
	template <typename T> T* GetModule(T* type);
	void ChildCreated(MdiChild* child);
private:
	MainWindow * m_mainWnd;
	QVector < iAModuleAction > m_moduleActions;
	QVector < iALoadedModule > m_loadedModules;
	
	iAModuleInterface* LoadModuleAndInterface(QFileInfo fi, iALogger* logger);
	void InitializeModuleInterface(iAModuleInterface* m);
};

template <typename T> T* iAModuleDispatcher::GetModule(T* type)
{
	for (iALoadedModule m: m_loadedModules.size())
	{
		T* ptr = dynamic_cast<T*>(m.moduleInterface);
		if (ptr)
		{
			return ptr;
		}
	}
	return 0;
}
