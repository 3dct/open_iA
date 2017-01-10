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
#include "iASmoothingModuleInterface.h"

#include "dlg_commoninput.h"
#include "iABlurring.h"
#include "iAEdgePreservingSmoothing.h"
#include "mainwindow.h"
#include "mdichild.h"

void iASmoothingModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSmoothing = getMenuWithTitle(filtersMenu, QString( "Smoothing" ) );
	QMenu * menuEdge_preserving_smoothing = getMenuWithTitle( menuSmoothing, QString( "Edge preserving smoothing" ) );
	QMenu * menuBlurring = getMenuWithTitle( menuSmoothing, QString( "Blurring" ) );
	menuSmoothing->addAction( menuEdge_preserving_smoothing->menuAction() );
	menuSmoothing->addAction( menuBlurring->menuAction() );

	QAction * actionGradient_anisotropic_diffusion = new QAction(QApplication::translate("MainWindow", "Gradient Anisotropic Diffusion", 0), m_mainWnd );
	QAction * actionDiscrete_Gaussian = new QAction(QApplication::translate("MainWindow", "Discrete Gaussian", 0), m_mainWnd );
	QAction * actionCurvature_Anisotropic_Diffusion = new QAction(QApplication::translate("MainWindow", "Curvature Anisotropic Diffusion", 0), m_mainWnd );
	QAction * actionBilateral = new QAction(QApplication::translate("MainWindow", "Bilateral Image Filter", 0), m_mainWnd );

	menuBlurring->addAction( actionDiscrete_Gaussian );
	menuEdge_preserving_smoothing->addAction( actionGradient_anisotropic_diffusion );
	menuEdge_preserving_smoothing->addAction( actionCurvature_Anisotropic_Diffusion );
	menuEdge_preserving_smoothing->addAction( actionBilateral );

	connect( actionGradient_anisotropic_diffusion, SIGNAL( triggered() ), this, SLOT( grad_aniso_diffusion() ) );
	connect( actionDiscrete_Gaussian, SIGNAL( triggered() ), this, SLOT( discrete_Gaussian_Filter() ) );
	connect( actionCurvature_Anisotropic_Diffusion, SIGNAL( triggered() ), this, SLOT( curv_aniso_diffusion() ) );
	connect( actionBilateral, SIGNAL( triggered() ), this, SLOT( bilat_filter() ) );
}

void iASmoothingModuleInterface::grad_aniso_diffusion()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Number of Iterations" ) << tr( "#Time Step" ) << tr( "#Conductance" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( gadIterations ) << tr( "%1" ).arg( gadTimeStep ) << tr( "%1" ).arg( gadConductance );
	dlg_commoninput dlg( m_mainWnd, "Gradient Anisotropic Diffusion", 3, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;
	gadIterations = dlg.getValues()[0]; gadTimeStep = dlg.getValues()[1]; gadConductance = dlg.getValues()[2];
	//prepare
	QString filterName = "Gradient anisotropic diffusion";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAEdgePreservingSmoothing* thread = new iAEdgePreservingSmoothing( filterName, GRADIENT_ANISOTROPIC_DIFFUSION,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setADParameters( gadIterations, gadTimeStep, gadConductance );
	thread->start();
}

void iASmoothingModuleInterface::discrete_Gaussian_Filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Variance" ) << tr( "#Maximum error" ) << tr( "$UShort Output" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( dgfVariance ) << tr( "%1" ).arg( dgfMaximumError ) << tr( "%1" ).arg( dgfOutput );
	dlg_commoninput dlg( m_mainWnd, "Discrete Gaussian", 3, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;
	dgfVariance = dlg.getValues()[0];
	dgfMaximumError = dlg.getValues()[1];
	dgfOutput = dlg.getCheckValues()[2];

	//prepare
	QString filterName = "Discrete Gaussian";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iABlurring* thread = new iABlurring( filterName, DISCRETE_GAUSSIAN,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDGParameters( dgfVariance, dgfMaximumError, dgfOutput );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASmoothingModuleInterface::curv_aniso_diffusion()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Number of Iterations" ) << tr( "#Time Step" ) << tr( "#Conductance" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( cadIterations ) << tr( "%1" ).arg( cadTimeStep ) << tr( "%1" ).arg( cadConductance );
	dlg_commoninput dlg( m_mainWnd, "Curvature Anisotropic Diffusion", 3, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;
	cadIterations = dlg.getValues()[0]; cadTimeStep = dlg.getValues()[1]; cadConductance = dlg.getValues()[2];
	//prepare
	QString filterName = "Curvature anisotropic diffusion";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAEdgePreservingSmoothing* thread = new iAEdgePreservingSmoothing( filterName, CURVATURE_ANISOTROPIC_DIFFUSION,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setADParameters( cadIterations, cadTimeStep, cadConductance );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASmoothingModuleInterface::bilat_filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Range Sigma" ) << tr( "#Domain Sigma" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( bilRangeSigma ) << tr( "%1" ).arg( bilDomainSigma );
	dlg_commoninput dlg( m_mainWnd, "Bilateral Image Filter", 2, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	bilRangeSigma = dlg.getValues()[0]; bilDomainSigma = dlg.getValues()[1];
	//prepare
	QString filterName = "Bilateral filtered";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAEdgePreservingSmoothing* thread = new iAEdgePreservingSmoothing( filterName, BILATERAL,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setBParameters( bilRangeSigma, bilDomainSigma );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

