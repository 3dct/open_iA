// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QList>

class iAVisModule;
class iAVisModuleItem;

class iAVisModulesCollection
{
public:
								iAVisModulesCollection( );
	int							addModule( iAVisModule * module, QString name );
	bool						removeModule( int id );
	QList<iAVisModuleItem *>	getModules( );
	QList<iAVisModuleItem *>	getModulesByStage( int stage );

private:
	QList<iAVisModuleItem *>	m_modules;
	int							m_currentId;
};
