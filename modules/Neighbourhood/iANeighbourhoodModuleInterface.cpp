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
#include "iANeighbourhoodModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAMedianFilter.h"
#include "mainwindow.h"
#include "mdichild.h"

void iANeighbourhoodModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuNeighbourhood = getMenuWithTitle(filtersMenu, QString("Neighbourhood"));
	QAction * actionItkMedianFilter = new QAction(QApplication::translate("MainWindow", "Median Filter", 0), m_mainWnd);
	menuNeighbourhood->addAction(actionItkMedianFilter);
	connect(actionItkMedianFilter, SIGNAL(triggered()), this, SLOT(median_Filter()));
}

void iANeighbourhoodModuleInterface::median_Filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#medR_x" ) << tr( "#medR_y" ) << tr( "#medR_z" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( medR_x ) << tr( "%1" ).arg( medR_y ) << tr( "%1" ).arg( medR_z );
	QTextDocument *fDescr = new QTextDocument( 0 );
	fDescr->setHtml(
		"<p><font size=+1>Applies a median filter to the volume.</font></p>"
		"<p>Computes an image where a given voxel is the median value of the the voxels "
		"in a neighborhood about the corresponding input voxel. A median filter is one of "
		"the family of nonlinear filters. It is used to smooth an image without being "
		"biased by outliers or shot noise.</p>"
		"<p>MedR defines the radius of the kernel in x,y,z direction.</p>" );

	dlg_commoninput dlg( m_mainWnd, "Median Filter Neighborhood Setting", 3, inList, inPara, fDescr );

	if( dlg.exec() != QDialog::Accepted )
		return;

	medR_x = dlg.getValues()[0];
	medR_y = dlg.getValues()[1];
	medR_z = dlg.getValues()[2];

	//prepare
	QString filterName = "Median Filtered";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAMedianFilter* thread = new iAMedianFilter( filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDParameters( medR_x, medR_y, medR_z );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
