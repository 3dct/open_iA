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
#include "iA4DCTCurrentVisualizationsDockWidget.h"
// iA
#include "iAVisModulesCollection.h"
#include "iAVisModuleItem.h"
#include "iAVisModule.h"
#include "iAVisModule.h"
// Qt
#include <QStringListModel>
#include <QItemSelectionModel>

iA4DCTCurrentVisualizationsDockWidget::iA4DCTCurrentVisualizationsDockWidget( QWidget * parent )
	: QDockWidget( parent )
	, m_previewEnabled( false )
	, m_currentStage( 0 )
	, m_collection( nullptr )
	, m_model( new QStringListModel )
	, m_currentVisModule( nullptr )
{
	setupUi( this );

	lvCurrentVisModules->setModel( m_model );
	connect( lvCurrentVisModules->selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ), this, SLOT( selectionChanged( QItemSelection, QItemSelection ) ) );
	connect( pbRemove, SIGNAL( clicked() ), this, SLOT( onRemoveButtonClicked() ) );
}

iA4DCTCurrentVisualizationsDockWidget::~iA4DCTCurrentVisualizationsDockWidget()
{
	delete m_model;
}

void iA4DCTCurrentVisualizationsDockWidget::setCurrentStage( int stage )
{
	m_currentStage = stage;
	sbPreview->setValue( m_currentStage );
	updateContext();
}

void iA4DCTCurrentVisualizationsDockWidget::setCollection( iAVisModulesCollection * collection )
{
	m_collection = collection;
}

void iA4DCTCurrentVisualizationsDockWidget::updateContext()
{
	if( m_collection == nullptr )
		return;

	m_visModulesNameList.clear();
	m_visModulesList = m_collection->getModulesByStage( m_currentStage );
	for( auto item : m_visModulesList ) {
		m_visModulesNameList << item->name;
	}
	m_model->setStringList( m_visModulesNameList );
}

void iA4DCTCurrentVisualizationsDockWidget::selectionChanged( const QItemSelection & selected, const QItemSelection & deselected )
{
	m_currentVisModule = nullptr;
	if( selected.size() == 0 ) {
		emit selectedVisModule( nullptr );
	}
	else if( selected.size() == 1 ) {
		m_currentVisModule = m_visModulesList[ selected.indexes()[0].row() ];
		emit selectedVisModule( m_currentVisModule->module );
	}
}

void iA4DCTCurrentVisualizationsDockWidget::onRemoveButtonClicked()
{
	if( m_currentVisModule != nullptr ) {
		m_currentVisModule->stages.remove( m_currentStage );
		m_currentVisModule = nullptr;
		emit selectedVisModule( nullptr );
	}
	updateContext();
	emit removedVisModule();
}
