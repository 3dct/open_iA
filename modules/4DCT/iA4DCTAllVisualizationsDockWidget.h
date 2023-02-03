// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTAllVisualizationsDockWidget.h"
// Qt
#include <QDockWidget>
#include <QStandardItemModel>
#include <QStringList>

class iAVisModulesCollection;
class iAVisModuleItem;
class iAVisModule;

class iA4DCTAllVisualizationsDockWidget : public QDockWidget, public Ui::AllVisualizationsDockWidget
{
	Q_OBJECT

public:
				iA4DCTAllVisualizationsDockWidget( QWidget * parent );
				~iA4DCTAllVisualizationsDockWidget( );
	void		setCollection( iAVisModulesCollection * collection );
	void		updateContext( );
	void		setCurrentStage( int currentStage );


signals:
	void		updateVisualizations( );
	void		selectedVisModule( iAVisModule* );


private:
	iAVisModuleItem *			getSelectedModule( );

	iAVisModulesCollection *	m_collection;
	QStandardItemModel			m_standardItemModel;
	QStringList					m_visModulesList;
	int							m_currentStage;


private slots:
	void		selectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
	void		onDeleteButtonClicked( );
	void		itemChanged( QStandardItem * item );
};
