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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#ifndef IA4DCTCURRENTVISUALIZATIONSDOCKWIDGET_H
#define IA4DCTCURRENTVISUALIZATIONSDOCKWIDGET_H
// Ui
#include "ui_iA4DCTCurrentVisualizationsDockWidget.h"
// Qt
#include <QDockWidget>
#include <QStringList>

class iAVisModulesCollection;
class QStringListModel;
class iAVisModuleItem;
class iAVisModule;

class iA4DCTCurrentVisualizationsDockWidget : public QDockWidget, public Ui::CurrentVisualizationsDockWidget
{
	Q_OBJECT
public:
					iA4DCTCurrentVisualizationsDockWidget( QWidget * parent );
					~iA4DCTCurrentVisualizationsDockWidget();
	void			setCollection( iAVisModulesCollection * collection );
	void			setCurrentStage( int stage );
	void			updateContext();

signals:
	void			selectedVisModule( iAVisModule * visModule );
	void			removedVisModule();

private:
	int							m_currentStage;
	bool						m_previewEnabled;
	iAVisModuleItem *			m_currentVisModule;
	iAVisModulesCollection *	m_collection;
	QStringListModel *			m_model;
	QStringList					m_visModulesNameList;
	QList<iAVisModuleItem *>	m_visModulesList;

private slots:
	void			selectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
	void			onRemoveButtonClicked();
};

#endif // IA4DCTCURRENTVISUALIZATIONSDOCKWIDGET_H