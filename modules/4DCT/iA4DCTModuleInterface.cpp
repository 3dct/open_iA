/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iA4DCTModuleInterface.h"

#include "iA4DCTMainWin.h"
#include "iA4DCTSettings.h"
#include "iAFeatureExtraction.h"
#include "iAFeatureExtractionDialog.h"
#include "iADefectClassifier.h"
#include "iAClassifyDefectsDialog.h"

#include <mainwindow.h>

#include <vtkMath.h>

#include <itkConvolutionImageFilter.h>
#include <itkEllipseSpatialObject.h>
#include <itkImageFileWriter.h>
#include <itkImageKernelOperator.h>
#include <itkImageToVTKImageFilter.h>
#include <itkLabelGeometryImageFilter.h>
#include <itkNormalizedCorrelationImageFilter.h>
#include <itkSpatialObjectToImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkVTKImageToImageFilter.h>

#include <QColor>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QObject>
#include <QSettings>

#include <limits>

#define RAD_TO_DEG 57.295779513082320876798154814105

iA4DCTModuleInterface::iA4DCTModuleInterface( )
{ /* not implemented */ }

iA4DCTModuleInterface::~iA4DCTModuleInterface( )
{ /* not implemented */ }

void iA4DCTModuleInterface::Initialize( )
{
	if (!m_mainWnd)
		return;
	QMenu* toolsMenu = m_mainWnd->toolsMenu( );
	QMenu* menu4DCT = getMenuWithTitle(toolsMenu, tr("4DCT"), false);

	QAction * newProj = new QAction(tr("New 4DCT project"), nullptr );
	newProj->setShortcut(QKeySequence( Qt::ALT + Qt::Key_4, Qt::Key_N ));
	connect( newProj, SIGNAL( triggered() ), this, SLOT( newProj() ) );
	menu4DCT->addAction( newProj );

	QAction * openProj = new QAction(tr("Open 4DCT project"), nullptr );
	openProj->setShortcut(QKeySequence( Qt::ALT + Qt::Key_4, Qt::Key_O ));
	connect( openProj, SIGNAL( triggered() ), this, SLOT( openProj() ) );
	menu4DCT->addAction( openProj );

	QAction* saveProj = new QAction(tr("Save 4DCT project"), nullptr);
	saveProj->setShortcut(QKeySequence( Qt::ALT + Qt::Key_4, Qt::Key_S ));
	connect( saveProj, SIGNAL( triggered() ), this, SLOT( saveProj() ) );
	menu4DCT->addAction( saveProj );

	QAction* featureExtraction = new QAction(tr("Extract features to file"), nullptr);
	connect( featureExtraction, SIGNAL( triggered() ), this, SLOT( extractFeaturesToFile() ) );
	menu4DCT->addAction( featureExtraction );

	QAction* defectClassification = new QAction(tr("Defect classification"), nullptr);
	connect( defectClassification, SIGNAL( triggered() ), this, SLOT( defectClassification() ) );
	menu4DCT->addAction( defectClassification );
}

/*============	Slots  ============*/

void iA4DCTModuleInterface::openProj( )
{
	QSettings settings;
	QString fileName = QFileDialog::getOpenFileName(
		m_mainWnd,
		tr( "Open 4DCT proj" ),
		settings.value( S_4DCT_OPEN_DIR ).toString( ),
		tr( "4DCT project (*.xml)" ) );

	QFileInfo file( fileName );
	if( !file.exists( ) ) {
		return;
	}
	settings.setValue( S_4DCT_OPEN_DIR, file.absolutePath( ) );

	iA4DCTMainWin* sv = new iA4DCTMainWin( m_mainWnd );
	sv->load( fileName );
	m_mainWnd->mdiArea->addSubWindow( sv );
	sv->show( );
}

void iA4DCTModuleInterface::newProj( )
{
	iA4DCTMainWin* sv = new iA4DCTMainWin( m_mainWnd );
	m_mainWnd->mdiArea->addSubWindow( sv );
	sv->show( );
}

void iA4DCTModuleInterface::saveProj( )
{
	QMdiSubWindow* subWnd = m_mainWnd->mdiArea->currentSubWindow( );
	iA4DCTMainWin* stackView = qobject_cast<iA4DCTMainWin*>( subWnd->widget( ) );
	if( stackView != nullptr ) {
		stackView->save( );
	}
}

void iA4DCTModuleInterface::extractFeaturesToFile( )
{
	iAFeatureExtractionDialog dialog;
	if( dialog.exec( ) == QDialog::Rejected )
	{
		return;
	}

	iAFeatureExtraction::run( dialog.getInputImg( ), dialog.getOutputFile( ) );
}

void iA4DCTModuleInterface::defectClassification()
{
	iAClassifyDefectsDialog dlg;
	if( dlg.exec( ) == QDialog::Rejected )
	{
		return;
	}

	iADefectClassifier df;
	iADefectClassifier::Parameters params;
	params.Spacing = dlg.ui.dsbSpacing->value( );
	params.ElongationP = dlg.ui.dsbElongationP->value( );
	params.ElongationD = dlg.ui.dsbElongationD->value( );
	params.LengthRangeP[0] = dlg.ui.dsbLengthRangeP_1->value( );
	params.LengthRangeP[1] = dlg.ui.dsbLengthRangeP_2->value( );
	params.WidthRangeP[0] = dlg.ui.dsbWidthRangeP_1->value( );
	params.WidthRangeP[1] = dlg.ui.dsbWidthRangeP_2->value( );
	params.AngleP = dlg.ui.dsbAngleP->value( );
	params.AngleB = dlg.ui.dsbAngleB->value( );
	params.AngleD = dlg.ui.dsbAngleD->value( );
	params.NeighborhoodDistP = dlg.ui.dsbNeighborhoodDistanceP->value( );
	params.NeighborhoodDistFF = dlg.ui.dsbNeighborhoodDistanceFF->value( );
	params.BigVolumeThreshold = dlg.ui.dsbBigVolumeThreshold->value( );
	params.FibersFile = dlg.ui.Fibers->ui.Path->text( );
	params.FeaturesFile = dlg.ui.Defects->ui.Path->text( );
	params.OutputDir = dlg.ui.Output->ui.Path->text( );

	df.run( params );
}

//void iA4DCTModuleInterface::enableDensityMap()
//{
//	PrepareActiveChild();
//	/*m_densityMap = new dlg_densityMap(m_mainWnd, m_mdiChild);
//	m_mdiChild->tabifyDockWidget(m_mdiChild->logDockWidget, m_densityMap);*/
//
//	dlg_4dctRegistration* reg = new dlg_4dctRegistration();
//	m_mainWnd->addSubWindow(reg);
//	reg->show();
//
//	QList<QMdiSubWindow*> list = m_mainWnd->MdiChildList();
//	foreach(QMdiSubWindow* window, m_mainWnd->MdiChildList())
//	{
//		MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
//		mdiChild->slicer(iASlicerMode::XY)->set4DCTRegistration(reg);
//		mdiChild->slicer(iASlicerMode::XZ)->set4DCTRegistration(reg);
//		mdiChild->slicer(iASlicerMode::YZ)->set4DCTRegistration(reg);
//	}
//}
