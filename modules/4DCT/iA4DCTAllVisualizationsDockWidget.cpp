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
#include "iA4DCTAllVisualizationsDockWidget.h"
// iA
#include "iAVisModulesCollection.h"
#include "iAVisModuleItem.h"
#include "iAVisModule.h"
// Qt
#include <QStringListModel>
#include <QStringList>
#include <QModelIndex>
#include <QVector>
#include <QMessageBox>

iA4DCTAllVisualizationsDockWidget::iA4DCTAllVisualizationsDockWidget( QWidget* parent )
	: QDockWidget( parent )
	, m_collection( nullptr )
	, m_currentStage( 0 )
	, m_visModulesModel( new QStringListModel )
{
	setupUi( this );
	connect( pbAdd, SIGNAL( clicked() ), this, SLOT( onAddButtonClicked() ) );
	connect( pbDelete, SIGNAL( clicked() ), this, SLOT( onDeleteButtonClicked() ) );
}

iA4DCTAllVisualizationsDockWidget::~iA4DCTAllVisualizationsDockWidget()
{
	delete m_visModulesModel;
}

void iA4DCTAllVisualizationsDockWidget::setCollection( iAVisModulesCollection* collection )
{
	m_collection = collection;
	updateContext();	
}

void iA4DCTAllVisualizationsDockWidget::updateContext()
{
	if( m_collection == nullptr )
		return;

	m_visModulesList.clear();
	for( auto m : m_collection->getModules() ) {
		m_visModulesList << m->name;
	}
	m_visModulesModel->setStringList( m_visModulesList );
	lvVisModules->setModel( m_visModulesModel );
}

void iA4DCTAllVisualizationsDockWidget::onAddButtonClicked()
{
	/*QModelIndexList indexList = lvVisModules->selectionModel()->selectedIndexes();
	for( auto index : indexList ) {
		iAVisModuleItem * moduleItem = m_collection->getModules().at( index.row() );
		moduleItem->stages.append( m_currentStage );
	}*/
	iAVisModuleItem * moduleItem = getSelectedModule();
	if( moduleItem == nullptr ) return;
	moduleItem->stages.append( m_currentStage );
	emit addedVisualization();
}

void iA4DCTAllVisualizationsDockWidget::setCurrentStage(int currentStage)
{
	m_currentStage = currentStage;
}

void iA4DCTAllVisualizationsDockWidget::onDeleteButtonClicked()
{
	iAVisModuleItem * moduleItem = getSelectedModule();
	if( moduleItem == nullptr ) return;
	if( moduleItem->stages.size() > 0 ) {
		QMessageBox::warning( this, "Can't remove the module", "The visualization module is used. Please first delete it from all stages.");
		return;
	}
	m_collection->removeModule( moduleItem->id );
	updateContext();
}

iAVisModuleItem * iA4DCTAllVisualizationsDockWidget::getSelectedModule()
{
	QModelIndexList indexList = lvVisModules->selectionModel()->selectedIndexes();
	for( auto index : indexList ) {
		return m_collection->getModules().at( index.row() );
	}
	return nullptr;
}
