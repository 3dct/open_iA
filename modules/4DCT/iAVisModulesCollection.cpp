// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVisModulesCollection.h"
// iA
#include "iAVisModule.h"
#include "iAVisModuleItem.h"

iAVisModulesCollection::iAVisModulesCollection( )
	: m_currentId( 0 )
{ /* not implemented */ }

int iAVisModulesCollection::addModule( iAVisModule * module, QString name )
{
	int id = m_currentId++;
	iAVisModuleItem * item = new iAVisModuleItem( module, name, id );
	m_modules.push_back( item );
	return id;
}

bool iAVisModulesCollection::removeModule( int id )
{
	for( int i = 0; i < m_modules.size( ); i++ ) {
		if( m_modules[i]->id == id ) {
			m_modules.removeAt( i );
			return true;
		}
	}
	return false;
}

QList<iAVisModuleItem *> iAVisModulesCollection::getModules( )
{
	return m_modules;
}

QList<iAVisModuleItem *> iAVisModulesCollection::getModulesByStage( int stage )
{
	QList<iAVisModuleItem *> result;
	for( auto m : m_modules ) {
		if( m->stages.indexOf( stage ) != -1 ) {
			result.push_back( m );
			continue;
		}
	}
	return result;
}
