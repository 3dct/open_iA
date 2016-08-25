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
#include "iAGPU_GradientAnisotropicDiffusionModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAGPUEdgePreservingSmoothing.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAGPU_GradientAnisotropicDiffusionModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSmoothing = getMenuWithTitle(filtersMenu, QString( "Smoothing" ) );
	QMenu * menuEdge_preserving_smoothing = getMenuWithTitle( menuSmoothing, QString( "Edge preserving smoothing" ) );

	QAction * actionGPU_Gradient_Anisotropic_Diffusion = new QAction( m_mainWnd );
	actionGPU_Gradient_Anisotropic_Diffusion->setText(QApplication::translate("MainWindow", "GPU Gradient Anisotropic Diffusion", 0));

	menuEdge_preserving_smoothing->addAction(actionGPU_Gradient_Anisotropic_Diffusion );
	connect( actionGPU_Gradient_Anisotropic_Diffusion, SIGNAL( triggered() ), this, SLOT( gpu_grad_aniso_diffusion() ) );
}

void iAGPU_GradientAnisotropicDiffusionModuleInterface::gpu_grad_aniso_diffusion()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Number of Iterations" ) << tr( "#Time Step" ) << tr( "#Conductance" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( gadIterations ) << tr( "%1" ).arg( gadTimeStep ) << tr( "%1" ).arg( gadConductance );
	dlg_commoninput dlg( m_mainWnd, "GPU Gradient Anisotropic Diffusion", 3, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	gadIterations = dlg.getValues()[0];
	gadTimeStep = dlg.getValues()[1];
	gadConductance = dlg.getValues()[2];
	//prepare
	QString filterName = tr( "GPU Gradient anisotropic diffusion filter" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAGPUEdgePreservingSmoothing * thread = new iAGPUEdgePreservingSmoothing( filterName, GPU_GRADIENT_ANISOTROPIC_DIFFUSION,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setADParameters( gadIterations, gadTimeStep, gadConductance );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
