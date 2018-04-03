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

#include "pch.h"
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
