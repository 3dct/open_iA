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
 
#include "pch.h"
#include "iAConnectedComponentModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAConnectedComponentFilters.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QFileDialog>
#include <QMessageBox>

void iAConnectedComponentModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuConnected_Component_Filters = getMenuWithTitle(filtersMenu, QString( "Connected Component Filters" ) );

	QAction * actionSimple_Connected_Component_Filter = new QAction( m_mainWnd );
	actionSimple_Connected_Component_Filter->setText( QApplication::translate( "MainWindow", "Simple Connected Component Filter", 0 ) );
	menuConnected_Component_Filters->addAction( actionSimple_Connected_Component_Filter );
	connect(actionSimple_Connected_Component_Filter, SIGNAL(triggered()), this, SLOT(simple_connected_component()));

	QAction * actionScalar_Connected_Component_Filter = new QAction( m_mainWnd );
	actionScalar_Connected_Component_Filter->setText( QApplication::translate( "MainWindow", "Scalar Connected Component Filter", 0 ) );
	menuConnected_Component_Filters->addAction( actionScalar_Connected_Component_Filter );
	connect(actionScalar_Connected_Component_Filter, SIGNAL(triggered()), this, SLOT(scalar_connected_component()));
	
	QAction * actionSimple_Relabel_Connected_Component_Filter = new QAction( m_mainWnd );
	actionSimple_Relabel_Connected_Component_Filter->setText( QApplication::translate( "MainWindow", "Simple Relabel Connected Component Filter", 0 ) );
	menuConnected_Component_Filters->addAction( actionSimple_Relabel_Connected_Component_Filter );
	connect( actionSimple_Relabel_Connected_Component_Filter, SIGNAL( triggered() ), this, SLOT( simple_relabel_connected_component() ) );
}

void iAConnectedComponentModuleInterface::simple_connected_component()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "$Fully Connected" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( sccff );
	dlg_commoninput dlg( m_mainWnd, "Simple Connected Component Filter", 1, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted ) 
		return;
	sccff = dlg.getCheckValues()[0];
	//prepare
	QString filterName = "Simple Connected Component";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAConnectedComponentFilters* thread = new iAConnectedComponentFilters( filterName, SIMPLE_CONNECTED_COMPONENT_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setSCCFParameters( sccff );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAConnectedComponentModuleInterface::scalar_connected_component()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "^Distance Threshold" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( scalccDistThresh );
	dlg_commoninput dlg( m_mainWnd, "Scalar Connected Component Filter", 1, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	scalccDistThresh = dlg.getDoubleSpinBoxValues()[0];
	//prepare
	QString filterName = "Scalar Connected Component";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAConnectedComponentFilters* thread = new iAConnectedComponentFilters( filterName, SCALAR_CONNECTED_COMPONENT_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setScalarCCFParameters( scalccDistThresh );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAConnectedComponentModuleInterface::simple_relabel_connected_component()
{
	//set filter description
	QTextDocument *fDescr = new QTextDocument();
	fDescr->setHtml(
		"<p>Remaps the labels associated with the objects in an image (as from the output of ConnectedComponentImageFilter) " 
		"such that the label numbers are consecutive with no gaps between the label numbers used. By default, the relabeling " 
		"will also sort the labels based on the size of the object: the largest object will have label #1, the second largest " 
		"will have label #2, etc. If two labels have the same size their initial order is kept. Label #0 is assumed to be the " 
		"background and is left unaltered by the relabeling.</p>"
		"<p>If user sets a minimum object size, all objects with fewer pixels than the minimum will be discarded, so that the "
		"number of objects reported will be only those remaining.</p>"
		"<p>Enabling the write option will save the file to a user specific path.</p>" );

	//set parameters
	QStringList inList = ( QStringList() << tr( "*Minimum Object Size" ) << tr( "$Write labels to file" ) );
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( srcifs ) << tr( "%1" ).arg( srcifw );
	dlg_commoninput dlg( m_mainWnd, "Simple Relabel Connected Component Filter", 2, inList, inPara, fDescr );
	if ( dlg.exec() != QDialog::Accepted )
		return;
	srcifs = dlg.getSpinBoxValues()[0];
	srcifw = dlg.getCheckValues()[1];

	//prepare
	QString filterName = "Simple Relabel Connected Component";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	QString filepatch = m_mainWnd->getPath(); //default

	if ( srcifw )
	{
		filepatch = QFileDialog::getSaveFileName( 0, tr( "Save file" ), 0, tr( "txt Files (*.txt *.TXT)" ) );
		if ( filepatch.isEmpty() )
		{
			QMessageBox msgBox;
			msgBox.setText( "No destination file was specified!" );
			msgBox.setWindowTitle( filterName );
			msgBox.exec();
			return;
		}
	}
	
	//execute
	iAConnectedComponentFilters* thread = new iAConnectedComponentFilters( filterName, SIMPLE_RELABEL_COMPONENT_IMAGE_FILTER,
																		   m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setSRCIFParameters( srcifw, srcifs, filepatch );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
