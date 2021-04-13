/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

// Ui
#include "ui_iA4DCTAllVisualizationsDockWidget.h"
// Qt
#include <QDockWidget>
#include <QStandardItemModel>

#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
class QStringList;
#else
#include <QList>
using QStringList = QList<QString>;
#endif

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