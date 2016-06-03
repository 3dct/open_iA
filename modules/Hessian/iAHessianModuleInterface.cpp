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
#include "iAHessianModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAHessianEigenanalysis.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAHessianModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menucomputeHessianEigenanalysis = getMenuWithTitle(filtersMenu, QString( "Hessian and Eigenanalysis" ) );
	QAction * actioncomputeHessianEigenanalysis = new QAction(QApplication::translate("MainWindow", "Compute Hessian and Eigenanalysis", 0), m_mainWnd);
	QAction * actioncomputeHessianEigenanalysis1 = new QAction(QApplication::translate("MainWindow", "Eigenanalysis Lambda1", 0), m_mainWnd);
	QAction * actioncomputeHessianEigenanalysis2 = new QAction(QApplication::translate("MainWindow", "Eigenanalysis Lambda2", 0), m_mainWnd);
	QAction * actioncomputeHessianEigenanalysis3 = new QAction(QApplication::translate("MainWindow", "Eigenanalysis Lambda3", 0), m_mainWnd);
	QAction * actioncomputeLaplacian = new QAction(QApplication::translate("MainWindow", "Compute Laplacian of Gaussian", 0), m_mainWnd);

	menucomputeHessianEigenanalysis->addAction( actioncomputeHessianEigenanalysis );
	menucomputeHessianEigenanalysis->addAction( actioncomputeHessianEigenanalysis1 );
	menucomputeHessianEigenanalysis->addAction( actioncomputeHessianEigenanalysis2 );
	menucomputeHessianEigenanalysis->addAction( actioncomputeHessianEigenanalysis3 );
	menucomputeHessianEigenanalysis->addSeparator();
	menucomputeHessianEigenanalysis->addAction(actioncomputeLaplacian); 

	connect( actioncomputeHessianEigenanalysis, SIGNAL( triggered() ), this, SLOT( computeHessianEigenanalysis() ) );
	connect( actioncomputeHessianEigenanalysis1, SIGNAL( triggered() ), this, SLOT( computeHessianEigenanalysis1() ) );
	connect( actioncomputeHessianEigenanalysis2, SIGNAL( triggered() ), this, SLOT( computeHessianEigenanalysis2() ) );
	connect( actioncomputeHessianEigenanalysis3, SIGNAL( triggered() ), this, SLOT( computeHessianEigenanalysis3() ) );
	connect( actioncomputeLaplacian,			 SIGNAL( triggered()),  this, SLOT( computeLaplacian			() ) ); 
}

void iAHessianModuleInterface::computeHessianEigenanalysis( int nr )
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Sigma" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( chefSigma );

	dlg_commoninput dlg( m_mainWnd, "Parameters for the Hessian matrix", 1, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;

	chefSigma = dlg.getValues()[0];
	//prepare
	QString filterName = tr( "Computing Hessian and Eigenanalyis" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAHessianEigenanalysis* thread = new iAHessianEigenanalysis( filterName, COMPUTEHESSIANEIGENANALYSIS,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	m_mdiChild->addMsg( "----------" );
	m_mdiChild->addMsg( m_mdiChild->currentFile() );
	thread->setCParameters( chefSigma, m_mdiChild->isHessianComputed(), nr );
	thread->start();
	m_mdiChild->setHessianComputed( true );
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAHessianModuleInterface::computeHessianEigenanalysis()
{
	computeHessianEigenanalysis( 0 );
}

void iAHessianModuleInterface::computeHessianEigenanalysis1()
{
	computeHessianEigenanalysis( 1 );
}

void iAHessianModuleInterface::computeHessianEigenanalysis2()
{
	computeHessianEigenanalysis( 2 );
}

void iAHessianModuleInterface::computeHessianEigenanalysis3()
{
	computeHessianEigenanalysis( 3 );
}


void iAHessianModuleInterface::computeLaplacian()
{

	//set parameters
	QStringList inList = (QStringList() << tr("#Sigma"));
	QList<QVariant> inPara; 	inPara << tr("%1").arg(chefSigma);

	dlg_commoninput dlg(m_mainWnd, "Parameters for the Laplacian matrix", 1, inList, inPara, NULL);


	if (dlg.exec() != QDialog::Accepted)
		return;
	chefSigma = dlg.getValues()[0];

	//prepare
	QString filterName = tr("Computing Laplacian of Gaussian");
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);

	//execute
	iAHessianEigenanalysis* thread = new iAHessianEigenanalysis(filterName, COMPUTE_LAPLACIAN,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);

	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->setLapParameters(chefSigma);

	m_mdiChild->addMsg(m_mdiChild->currentFile());
	thread->start();

	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}
