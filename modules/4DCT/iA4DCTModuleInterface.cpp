// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTModuleInterface.h"

#include "iA4DCTMainWin.h"
#include "iA4DCTSettings.h"
#include "iAFeatureExtraction.h"
#include "iAFeatureExtractionDialog.h"
#include "iADefectClassifier.h"
#include "iAClassifyDefectsDialog.h"

#include <iAMainWindow.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMenu>
#include <QSettings>


#define RAD_TO_DEG 57.295779513082320876798154814105

iA4DCTModuleInterface::iA4DCTModuleInterface( )
{ /* not implemented */ }

iA4DCTModuleInterface::~iA4DCTModuleInterface( )
{ /* not implemented */ }

void iA4DCTModuleInterface::Initialize( )
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(4DCT);
	QMenu* menu4DCT = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("4DCT"), false);

	QAction * newProj = new QAction(tr("New 4DCT project"), m_mainWnd);
	newProj->setShortcut(QKeySequence(QKeyCombination(Qt::ALT, Qt::Key_4), QKeyCombination(Qt::Key_N)));
	connect( newProj, &QAction::triggered, this, &iA4DCTModuleInterface::newProj);
	menu4DCT->addAction( newProj );

	QAction * openProj = new QAction(tr("Open 4DCT project"), m_mainWnd);
	openProj->setShortcut(QKeySequence(QKeyCombination(Qt::ALT, Qt::Key_4), QKeyCombination(Qt::Key_O)));
	connect( openProj, &QAction::triggered, this, &iA4DCTModuleInterface::openProj);
	menu4DCT->addAction( openProj );

	QAction* saveProj = new QAction(tr("Save 4DCT project"), m_mainWnd);
	saveProj->setShortcut(QKeySequence(QKeyCombination(Qt::ALT, Qt::Key_4), QKeyCombination(Qt::Key_S)));
	connect( saveProj, &QAction::triggered, this, &iA4DCTModuleInterface::saveProj);
	menu4DCT->addAction( saveProj );

	QAction* featureExtraction = new QAction(tr("Extract features to file"), m_mainWnd);
	connect( featureExtraction, &QAction::triggered, this, &iA4DCTModuleInterface::extractFeaturesToFile);
	menu4DCT->addAction( featureExtraction );

	QAction* defectClassification = new QAction(tr("Defect classification"), m_mainWnd);
	connect( defectClassification, &QAction::triggered, this, &iA4DCTModuleInterface::defectClassification);
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
	m_mainWnd->addSubWindow( sv );
	sv->show( );
}

void iA4DCTModuleInterface::newProj( )
{
	iA4DCTMainWin* sv = new iA4DCTMainWin( m_mainWnd );
	m_mainWnd->addSubWindow( sv );
	sv->show( );
}

void iA4DCTModuleInterface::saveProj( )
{
	QMdiSubWindow* subWnd = m_mainWnd->activeChild();
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
//	m_mdiChild->tabifyDockWidget(m_mdiChild->renderDockWidget(), m_densityMap);*/
//
//	dlg_4dctRegistration* reg = new dlg_4dctRegistration();
//	m_mainWnd->addSubWindow(reg);
//	reg->show();
//
//	QList<QMdiSubWindow*> list = m_mainWnd->iAMdiChildList();
//	foreach(QMdiSubWindow* window, m_mainWnd->iAMdiChildList())
//	{
//		iAMdiChild *mdiChild = qobject_cast<iAMdiChild *>(window->widget());
//		mdiChild->slicer(iASlicerMode::XY)->set4DCTRegistration(reg);
//		mdiChild->slicer(iASlicerMode::XZ)->set4DCTRegistration(reg);
//		mdiChild->slicer(iASlicerMode::YZ)->set4DCTRegistration(reg);
//	}
//}
