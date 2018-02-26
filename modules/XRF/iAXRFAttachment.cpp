/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAXRFAttachment.h"

#include "dlg_periodicTable.h"
#include "dlg_RefSpectra.h"
#include "dlg_SimilarityMap.h"
#include "dlg_XRF.h"
#include "iAElementConcentrations.h"
#include "iAXRFData.h"

#include "iAChannelVisualizationData.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iAWidgetAddHelper.h"
#include "io/extension2id.h"
#include "io/iAIO.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <qmath.h>

iAXRFAttachment::iAXRFAttachment( MainWindow * mainWnd, iAChildData childData ) : iAModuleAttachmentToChild( mainWnd, childData ), 
	dlgPeriodicTable(0), dlgXRF(0), dlgSimilarityMap(0), ioThread(0), slicerXZ(0), slicerXY(0),	slicerYZ(0)
{
	MdiChild * mdiChild = m_childData.child;
	connect( mdiChild, SIGNAL( magicLensToggled( bool ) ), this, SLOT( magicLensToggled( bool ) ) );
	slicerXZ = mdiChild->getSlicerXZ();
	slicerXY = mdiChild->getSlicerXY();
	slicerYZ = mdiChild->getSlicerYZ();
	connect( mdiChild->getSlicerDataXY(), SIGNAL( oslicerPos( int, int, int, int ) ), this, SLOT( updateXRFVoxelEnergy( int, int, int, int ) ) );
	connect( mdiChild->getSlicerDataXZ(), SIGNAL( oslicerPos( int, int, int, int ) ), this, SLOT( updateXRFVoxelEnergy( int, int, int, int ) ) );
	connect( mdiChild->getSlicerDataYZ(), SIGNAL( oslicerPos( int, int, int, int ) ), this, SLOT( updateXRFVoxelEnergy( int, int, int, int ) ) );
	//TODO: move
	if( !filter_SimilarityMap() )
		throw itk::ExceptionObject(__FILE__, __LINE__, "filter_SimilarityMap failed");
	QString filtername = tr( "XRF" );
	mdiChild->addStatusMsg( filtername );

	QString f = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr( "Open File" ),
		mdiChild->getFilePath(),
		tr( "All supported types (*.mhd *.raw *.volstack);;MetaImages (*.mhd *.mha);;RAW files (*.raw);;Volume Stack (*.volstack)" ) );
	if( !QFile::exists( f ) )
		throw itk::ExceptionObject(__FILE__, __LINE__, "File does not exist");

	mdiChild->addMsg(tr("%1  Loading file '%2', please wait...")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(f));

	dlgPeriodicTable = new dlg_periodicTable( mdiChild );
	mdiChild->splitDockWidget( mdiChild->sXZ, dlgPeriodicTable, Qt::Horizontal );

	dlgRefSpectra = new dlg_RefSpectra( mdiChild );
	mdiChild->splitDockWidget(mdiChild->sXY, dlgRefSpectra, Qt::Horizontal);
	
	dlgXRF = new dlg_XRF( mdiChild, dlgPeriodicTable, dlgRefSpectra );

	ioThread = new iAIO( mdiChild->getLogger(), mdiChild, dlgXRF->GetXRFData()->GetDataPtr() );
	mdiChild->setReInitializeRenderWindows( false );
	mdiChild->connectIOThreadSignals( ioThread );
	connect( ioThread, SIGNAL( done() ), this, SLOT( xrfLoadingDone() ) );
	connect( ioThread, SIGNAL( failed() ), this, SLOT( xrfLoadingFailed() ) );
	connect( ioThread, SIGNAL( finished() ), this, SLOT( ioFinished() ) );

	QString extension = QFileInfo( f ).suffix();
	extension = extension.toUpper();
	if( extensionToIdStack.find( extension ) == extensionToIdStack.end() )
		throw itk::ExceptionObject(__FILE__, __LINE__, "Unsupported extension");

	IOType id = extensionToIdStack.find( extension ).value();
	if( !ioThread->setupIO( id, f ) )
	{
		xrfLoadingFailed();
		throw itk::ExceptionObject(__FILE__, __LINE__, "XRF loading failed");
	}
	ioThread->start();
}


iAXRFAttachment::~iAXRFAttachment()
{}


void iAXRFAttachment::reInitXRF()
{	
	vtkSmartPointer<vtkImageData> img = dlgXRF->GetCombinedVolume();
	m_otf = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_otf->AddPoint(img->GetScalarRange()[0], 1);
	m_otf->AddPoint(img->GetScalarRange()[1], 1);
	m_childData.child->reInitMagicLens(ch_XRF, img, dlgXRF->GetColorTransferFunction(), m_otf);
}

void iAXRFAttachment::initXRF()
{
	initXRF( true );
}

void iAXRFAttachment::deinitXRF()
{
	initXRF( false );
}

void iAXRFAttachment::initXRF( bool enableChannel )
{
	iAChannelVisualizationData * chData = m_childData.child->GetChannelData( ch_XRF );
	if( !chData )
	{
		chData = new iAChannelVisualizationData();
		m_childData.child->InsertChannelData( ch_XRF, chData );
	}
	vtkSmartPointer<vtkImageData> img = dlgXRF->GetCombinedVolume();
	chData->SetImage( img );
	m_otf = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_otf->AddPoint(img->GetScalarRange()[0], 1);
	m_otf->AddPoint(img->GetScalarRange()[1], 1);
	chData->SetColorTF( dlgXRF->GetColorTransferFunction() );
	chData->SetOpacityTF(m_otf);
	chData->SetName("Spectral Color Image");
	m_childData.child->InitChannelRenderer( ch_XRF, false, enableChannel );
	bool isMagicLensEnabled = m_childData.child->isMagicLensToggled();
	if( enableChannel )
	{
		updateSlicerXRFOpacity();
	}
	else if (isMagicLensEnabled)
	{
		m_childData.child->SetMagicLensInput(ch_XRF,
			!m_childData.child->GetChannelData(ch_XRF)->IsEnabled());
	}
	m_childData.child->updateSlicers();
	m_childData.child->addMsg( tr( "%1  Spectral color image initialized." ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) ) );
}

QObject* iAXRFAttachment::recalculateXRF()
{
	if( !dlgXRF )
	{
		return 0;
	}
	return dlgXRF->UpdateForVisualization();
}

void iAXRFAttachment::updateXRFVoxelEnergy( int x, int y, int z, int mode )
{
	if( !dlgXRF )
	{
		return;
	}
	if( dlgXRF->cb_spectrumProbing->isChecked() )
	{
		iAXRFData * xrfData = dlgXRF->GetXRFData().data();
		if( !xrfData || xrfData->begin() == xrfData->end() )
		{
			return;
		}
		vtkSmartPointer<vtkImageData> firstImg = *xrfData->begin();
		double imgSpacing[3];
		m_childData.imgData->GetSpacing( imgSpacing );

		double xrfSpacing[3];
		firstImg->GetSpacing( xrfSpacing );

		double spacing[3];
		for( int i = 0; i<3; ++i )
		{
			spacing[i] = xrfSpacing[i] / imgSpacing[i];
		}

		int extent[6];
		firstImg->GetExtent( extent );

		int xrfX = qFloor( x / spacing[0] );
		int xrfY = qFloor( y / spacing[1] );
		int xrfZ = qFloor( z / spacing[2] );

		if( xrfX > extent[1] )
			xrfX = extent[1];
		if( xrfY > extent[3] )
			xrfY = extent[3];
		if( xrfZ > extent[5] )
			xrfZ = extent[5];

		dlgXRF->UpdateVoxelSpectrum( xrfX, xrfY, xrfZ );

		if( dlgXRF->isDecompositionLoaded() )
		{
			double * compositionSpacing = dlgXRF->GetElementConcentrations()->getImage( 0 )->GetSpacing();
			int concPos[3] = {
				qFloor( x*imgSpacing[0] / compositionSpacing[0] ),
				qFloor( y*imgSpacing[1] / compositionSpacing[1] ),
				qFloor( z*imgSpacing[2] / compositionSpacing[2] ) };
			dlgXRF->UpdateConcentrationViews( concPos[0], concPos[1], concPos[2] );
		}
	}
}

void iAXRFAttachment::xrfLoadingDone()
{
	double minEnergy = 0;
	double maxEnergy = dlgXRF->GetXRFData()->size();
	bool haveEnergyLevels = false;
	QString energyRange = ioThread->getAdditionalInfo();
	if( !energyRange.isEmpty() )
	{
		QStringList energies = energyRange.split( ":" );
		if( energies.size() == 2 )
		{
			minEnergy = energies[0].toDouble();
			maxEnergy = energies[1].toDouble();
			haveEnergyLevels = true;
		}
	}
	iAWidgetAddHelper wdgtHelp(m_childData.child, m_childData.logs);
	dlgXRF->init( minEnergy, maxEnergy, haveEnergyLevels, wdgtHelp);
	connect( dlgXRF->cb_spectralColorImage, SIGNAL( stateChanged( int ) ), this, SLOT( visualizeXRF( int ) ) );
	connect( dlgXRF->sl_peakOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( updateXRFOpacity( int ) ) );
	connect( dlgXRF->pb_compute, SIGNAL( clicked() ), this, SLOT( updateXRF() ) );
	m_childData.child->tabifyDockWidget( dlgRefSpectra, dlgXRF );
	dlgSimilarityMap->connectToXRF( dlgXRF );
	emit xrfLoaded();
	m_childData.child->updateLayout();
}

void iAXRFAttachment::xrfLoadingFailed()
{
	m_childData.child->addMsg( tr( "%1  XRF data loading has failed!" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) ) );
	delete dlgXRF;
	delete dlgPeriodicTable;
	delete dlgRefSpectra;
	delete dlgSimilarityMap;
	dlgXRF = NULL;
	dlgPeriodicTable = NULL;
	dlgRefSpectra = NULL;
	dlgSimilarityMap = NULL;
	emit detach();
}

void iAXRFAttachment::updateSlicerXRFOpacity()
{
	double opacity = (double)dlgXRF->sl_peakOpacity->value() / dlgXRF->sl_peakOpacity->maximum();
	m_childData.child->UpdateChannelSlicerOpacity( ch_XRF, opacity );
}

void iAXRFAttachment::updateXRFOpacity( int value )
{
	iAChannelVisualizationData * data = m_childData.child->GetChannelData( ch_XRF );
	if( data && data->IsEnabled() )
	{
		dlgXRF->sl_peakOpacity->repaint();
		updateSlicerXRFOpacity();
	}
}

bool iAXRFAttachment::filter_SimilarityMap()
{
	dlgSimilarityMap = new dlg_SimilarityMap( m_childData.child );
	m_childData.child->tabifyDockWidget( m_childData.logs, dlgSimilarityMap );

	return true;
}

void iAXRFAttachment::initSlicerXRF( bool enableChannel )
{
	assert( !m_childData.child->GetChannelData( ch_XRF ) );
	m_childData.child->addMsg( tr( "%1  Initializing Spectral Color Image. This may take a while..." ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) ) );
	QObject* calcThread = recalculateXRF();
	if( enableChannel )
	{
		QObject::connect( calcThread, SIGNAL( finished() ), this, SLOT( initXRF() ) );
	}
	else
	{
		QObject::connect( calcThread, SIGNAL( finished() ), this, SLOT( deinitXRF() ) );
	}
}

void iAXRFAttachment::visualizeXRF( int isOn )
{
	bool enabled = (isOn != 0);
	iAChannelVisualizationData * chData = m_childData.child->GetChannelData( ch_XRF );
	if( !chData && enabled )
	{
		initSlicerXRF( true );
		return;
	}
	m_childData.child->SetChannelRenderingEnabled( ch_XRF, enabled );
	if( enabled )
	{
		updateSlicerXRFOpacity();
	}
}

void iAXRFAttachment::updateXRF()
{
	iAChannelVisualizationData * chData = m_childData.child->GetChannelData( ch_XRF );
	bool isMagicLensEnabled = m_childData.child->isMagicLensToggled();
	if( !chData || (!chData->IsEnabled() && !isMagicLensEnabled) )
	{
		return;
	}
	QObject* calcThread = recalculateXRF();
	if( !calcThread )
	{
		return;
	}
	QObject::connect( calcThread, SIGNAL( finished() ), this, SLOT( reInitXRF() ) );
}

void iAXRFAttachment::magicLensToggled( bool isOn )
{
	if( dlgXRF && !m_childData.child->GetChannelData( ch_XRF ) )
	{
		initSlicerXRF( false );
		return;
	}
}

void iAXRFAttachment::ioFinished()
{
	ioThread = 0;
}
