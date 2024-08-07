// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTAllVisualizationsDockWidget.h"

#include "iAVisModulesCollection.h"
#include "iAVisModuleItem.h"
#include "iAVisModule.h"

#include <QModelIndex>
#include <QVector>
#include <QMessageBox>

iA4DCTAllVisualizationsDockWidget::iA4DCTAllVisualizationsDockWidget( QWidget* parent )
	: QDockWidget( parent )
	, m_collection( nullptr )
	, m_currentStage( 0 )
{
	setupUi( this );

	lvVisModules->setModel( &m_standardItemModel );

	connect( pbDelete, &QPushButton::clicked, this, &iA4DCTAllVisualizationsDockWidget::onDeleteButtonClicked);
	connect( &m_standardItemModel, &QStandardItemModel::itemChanged, this, &iA4DCTAllVisualizationsDockWidget::itemChanged);
	connect( lvVisModules->selectionModel(), &QItemSelectionModel::selectionChanged, this, &iA4DCTAllVisualizationsDockWidget::selectionChanged);
}

iA4DCTAllVisualizationsDockWidget::~iA4DCTAllVisualizationsDockWidget( )
{ }

void iA4DCTAllVisualizationsDockWidget::setCollection( iAVisModulesCollection * collection )
{
	m_collection = collection;
	updateContext( );
}

void iA4DCTAllVisualizationsDockWidget::updateContext( )
{
	if (m_collection == nullptr)
	{
		return;
	}

	m_standardItemModel.clear( );
	for (auto m : m_collection->getModules( ))
	{
		QStandardItem * item = new QStandardItem( m->name );
		item->setCheckable( true );
		if (m->stages.contains(m_currentStage))
		{
			item->setCheckState(Qt::Checked);
		}
		else
		{
			item->setCheckState(Qt::Unchecked);
		}
		m_standardItemModel.appendRow( item );
	}
}

void iA4DCTAllVisualizationsDockWidget::setCurrentStage( int currentStage )
{
	m_currentStage = currentStage;
}

void iA4DCTAllVisualizationsDockWidget::onDeleteButtonClicked( )
{
	iAVisModuleItem * moduleItem = getSelectedModule( );
	if (moduleItem == nullptr)
	{
		return;
	}
	if (moduleItem->stages.size( ) > 0 )
	{
		QMessageBox::warning( this, "Can't remove the module", "The visualization module is used. Please first delete it from all stages." );
		return;
	}
	m_collection->removeModule( moduleItem->id );
	updateContext( );
}

iAVisModuleItem * iA4DCTAllVisualizationsDockWidget::getSelectedModule( )
{
	QModelIndexList indexList = lvVisModules->selectionModel( )->selectedIndexes( );
	if (!indexList.isEmpty())
	{
		return nullptr;
	}
	return m_collection->getModules().at( indexList[0].row() );
}

void iA4DCTAllVisualizationsDockWidget::itemChanged( QStandardItem * item )
{
	const QModelIndex index = item->model( )->indexFromItem( item );
	iAVisModuleItem * module = m_collection->getModules( ).at( index.row( ) );
	module->name = item->text( );
	if (item->checkState( ) == Qt::Checked)
	{
		if (!module->stages.contains(m_currentStage))
		{
			module->stages.append(m_currentStage);
		}
	}
	else
	{
		if (module->stages.contains(m_currentStage))
		{

			module->stages.removeOne(m_currentStage);
		}
	}
	emit updateVisualizations( );
}

void iA4DCTAllVisualizationsDockWidget::selectionChanged( const QItemSelection & selected, const QItemSelection & /*deselected*/ )
{
	if (selected.size( ) == 0)
	{
		emit selectedVisModule( nullptr );
	}
	else if (selected.size( ) == 1)
	{
		QModelIndex index = selected.indexes( )[0];
		QStandardItem * item = m_standardItemModel.item( index.row( ), index.column( ) );
		if (item->checkState( ) != Qt::Checked)
		{
			emit selectedVisModule( nullptr );
		}
		else
		{
			iAVisModule * module = m_collection->getModules( )[index.row( )]->module;
			emit selectedVisModule( module );
		}
	}
}
