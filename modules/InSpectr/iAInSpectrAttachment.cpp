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
#include "iAInSpectrAttachment.h"

#include "dlg_periodicTable.h"
#include "dlg_RefSpectra.h"
#include "dlg_SimilarityMap.h"
#include "dlg_InSpectr.h"
#include "iAElementConcentrations.h"
#include "iAXRFData.h"

#include <dlg_slicer.h>
#include <iAChannelData.h>
#include <iASlicer.h>
#include <io/extension2id.h>
#include <io/iAIO.h>
#include <mainwindow.h>
#include <mdichild.h>
#include <qthelper/iAWidgetAddHelper.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QtMath>

iAInSpectrAttachment::iAInSpectrAttachment( MainWindow * mainWnd, MdiChild * child ) : iAModuleAttachmentToChild( mainWnd, child ),
	dlgPeriodicTable(nullptr),
	dlgSimilarityMap(nullptr),
	dlgXRF(nullptr),
	ioThread(nullptr),
	m_xrfChannelID(NotExistingChannel)
{
	connect(m_child, &MdiChild::magicLensToggled, this, &iAInSpectrAttachment::magicLensToggled);
	for (int i = 0; i < 3; ++i)
	{
		connect(m_child->slicer(i), &iASlicer::oslicerPos, this, &iAInSpectrAttachment::updateXRFVoxelEnergy);
	}
	//TODO: move
	if (!filter_SimilarityMap())
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "filter_SimilarityMap failed");
	}
	QString filtername = tr( "XRF" );
	m_child->addStatusMsg( filtername );

	QString f = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr( "Open File" ),
		m_child->filePath(),
		tr( "All supported types (*.mhd *.raw *.volstack);;MetaImages (*.mhd *.mha);;RAW files (*.raw);;Volume Stack (*.volstack)" ) );
	if (!QFile::exists(f))
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "File does not exist");
	}

	m_child->addMsg(tr("Loading file '%1', please wait...").arg(f));

	dlgPeriodicTable = new dlg_periodicTable( m_child );
	dlgRefSpectra = new dlg_RefSpectra( m_child );
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), dlgPeriodicTable, Qt::Horizontal);
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), dlgRefSpectra, Qt::Horizontal);

	dlgXRF = new dlg_InSpectr( m_child, dlgPeriodicTable, dlgRefSpectra );

	ioThread = new iAIO( m_child->logger(), m_child, dlgXRF->GetXRFData()->GetDataPtr() );
	m_child->setReInitializeRenderWindows( false );
	m_child->connectIOThreadSignals( ioThread );
	connect( ioThread, &iAIO::done, this, &iAInSpectrAttachment::xrfLoadingDone);
	connect( ioThread, &iAIO::failed, this, &iAInSpectrAttachment::xrfLoadingFailed);
	connect( ioThread, &iAIO::finished, this, &iAInSpectrAttachment::ioFinished);

	QString extension = QFileInfo( f ).suffix();
	extension = extension.toUpper();
	if (extensionToIdStack.find(extension) == extensionToIdStack.end())
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "Unsupported extension");
	}

	iAIOType id = extensionToIdStack.find( extension ).value();
	if( !ioThread->setupIO( id, f ) )
	{
		xrfLoadingFailed();
		throw itk::ExceptionObject(__FILE__, __LINE__, "XRF loading failed");
	}
	ioThread->start();
}


iAInSpectrAttachment::~iAInSpectrAttachment()
{}


void iAInSpectrAttachment::reInitXRF()
{
	vtkSmartPointer<vtkImageData> img = dlgXRF->GetCombinedVolume();
	if (m_child->isMagicLens2DEnabled())
	{
		m_child->reInitMagicLens(m_xrfChannelID, "Spectral Color Image", img, dlgXRF->GetColorTransferFunction());
	}
	if (m_child->channelData(m_xrfChannelID) && m_child->channelData(m_xrfChannelID)->isEnabled())
	{
		m_child->updateChannel(m_xrfChannelID, img, dlgXRF->GetColorTransferFunction(), nullptr, false);
	}
}

void iAInSpectrAttachment::initXRF()
{
	initXRF( true );
}

void iAInSpectrAttachment::deinitXRF()
{
	initXRF( false );
}

void iAInSpectrAttachment::initXRF( bool enableChannel )
{
	if (m_xrfChannelID == NotExistingChannel)
	{
		m_xrfChannelID = m_child->createChannel();
	}
	vtkSmartPointer<vtkImageData> img = dlgXRF->GetCombinedVolume();
	auto chData = m_child->channelData(m_xrfChannelID);
	chData->setImage( img );
	chData->setColorTF( dlgXRF->GetColorTransferFunction() );
	chData->setName("Spectral Color Image");
	// TODO: initialize channel?
	m_child->initChannelRenderer(m_xrfChannelID, false, enableChannel );
	bool isMagicLensEnabled = m_child->isMagicLens2DEnabled();
	if( enableChannel )
	{
		updateSlicerXRFOpacity();
	}
	else if (isMagicLensEnabled)
	{
		m_child->setMagicLensInput(m_xrfChannelID);
	}
	m_child->updateSlicers();
	m_child->addMsg(tr("Spectral color image initialized."));
}

QThread* iAInSpectrAttachment::recalculateXRF()
{
	if( !dlgXRF )
	{
		return nullptr;
	}
	return dlgXRF->UpdateForVisualization();
}

void iAInSpectrAttachment::updateXRFVoxelEnergy( int x, int y, int z, int /*mode*/ )
{
	if (!dlgXRF)
	{
		return;
	}
	if (dlgXRF->cb_spectrumProbing->isChecked())
	{
		iAXRFData * xrfData = dlgXRF->GetXRFData().data();
		if (!xrfData || xrfData->begin() == xrfData->end())
		{
			return;
		}
		vtkSmartPointer<vtkImageData> firstImg = *xrfData->begin();
		double imgSpacing[3];
		m_child->imageData()->GetSpacing( imgSpacing );

		double xrfSpacing[3];
		firstImg->GetSpacing( xrfSpacing );

		double spacing[3];
		for (int i = 0; i<3; ++i)
		{
			spacing[i] = xrfSpacing[i] / imgSpacing[i];
		}

		int extent[6];
		firstImg->GetExtent( extent );

		int xrfX = qFloor( x / spacing[0] );
		int xrfY = qFloor( y / spacing[1] );
		int xrfZ = qFloor( z / spacing[2] );

		if (xrfX > extent[1])
		{
			xrfX = extent[1];
		}
		if (xrfY > extent[3])
		{
			xrfY = extent[3];
		}
		if (xrfZ > extent[5])
		{
			xrfZ = extent[5];
		}

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

void iAInSpectrAttachment::xrfLoadingDone()
{
	double minEnergy = 0;
	double maxEnergy = dlgXRF->GetXRFData()->size();
	bool haveEnergyLevels = false;
	QString energyRange = ioThread->additionalInfo();
	if (!energyRange.isEmpty())
	{
		QStringList energies = energyRange.split( ":" );
		if (energies.size() == 2)
		{
			minEnergy = energies[0].toDouble();
			maxEnergy = energies[1].toDouble();
			haveEnergyLevels = true;
		}
	}
	iAWidgetAddHelper wdgtHelp(m_child, m_child->logDockWidget());
	dlgXRF->init( minEnergy, maxEnergy, haveEnergyLevels, wdgtHelp);
	connect( dlgXRF->cb_spectralColorImage, &QCheckBox::stateChanged, this, &iAInSpectrAttachment::visualizeXRF);
	connect( dlgXRF->sl_peakOpacity, &QSlider::valueChanged, this, &iAInSpectrAttachment::updateXRFOpacity);
	connect( dlgXRF->pb_compute, &QPushButton::clicked, this, &iAInSpectrAttachment::updateXRF);
	m_child->tabifyDockWidget( dlgRefSpectra, dlgXRF );
	dlgSimilarityMap->connectToXRF( dlgXRF );
	m_child->updateLayout();
}

void iAInSpectrAttachment::xrfLoadingFailed()
{
	m_child->addMsg( tr("XRF data loading has failed!"));
	delete dlgXRF;
	delete dlgPeriodicTable;
	delete dlgRefSpectra;
	delete dlgSimilarityMap;
	dlgXRF = nullptr;
	dlgPeriodicTable = nullptr;
	dlgRefSpectra = nullptr;
	dlgSimilarityMap = nullptr;
	emit detach();
}

void iAInSpectrAttachment::updateSlicerXRFOpacity()
{
	double opacity = (double)dlgXRF->sl_peakOpacity->value() / dlgXRF->sl_peakOpacity->maximum();
	m_child->updateChannelOpacity(m_xrfChannelID, opacity );
}

void iAInSpectrAttachment::updateXRFOpacity( int /*value*/ )
{
	iAChannelData * data = m_child->channelData(m_xrfChannelID);
	if (data && data->isEnabled())
	{
		dlgXRF->sl_peakOpacity->repaint();
		updateSlicerXRFOpacity();
	}
}

bool iAInSpectrAttachment::filter_SimilarityMap()
{
	dlgSimilarityMap = new dlg_SimilarityMap( m_child );
	m_child->tabifyDockWidget( m_child->logDockWidget(), dlgSimilarityMap );

	return true;
}

void iAInSpectrAttachment::initSlicerXRF( bool enableChannel )
{
	assert( !m_child->channelData(m_xrfChannelID) );
	m_child->addMsg(tr("Initializing Spectral Color Image. This may take a while..."));
	auto calcThread = recalculateXRF();
	if (enableChannel)
	{
		QObject::connect(calcThread, &QThread::finished, this, QOverload<>::of(&iAInSpectrAttachment::initXRF));
	}
	else
	{
		QObject::connect(calcThread, &QThread::finished, this, &iAInSpectrAttachment::deinitXRF);
	}
}

void iAInSpectrAttachment::visualizeXRF( int isOn )
{
	bool enabled = (isOn != 0);
	iAChannelData * chData = m_child->channelData(m_xrfChannelID);
	if (!chData && enabled)
	{
		initSlicerXRF( true );
		return;
	}
	m_child->setChannelRenderingEnabled(m_xrfChannelID, enabled );
	if (enabled)
	{
		updateSlicerXRFOpacity();
	}
}

void iAInSpectrAttachment::updateXRF()
{
	iAChannelData * chData = m_child->channelData(m_xrfChannelID);
	bool isMagicLensEnabled = m_child->isMagicLens2DEnabled();
	if (!chData || (!chData->isEnabled() && !isMagicLensEnabled))
	{
		return;
	}
	QThread* calcThread = recalculateXRF();
	if (!calcThread)
	{
		return;
	}
	connect( calcThread, &QThread::finished, this, &iAInSpectrAttachment::reInitXRF);
	connect( calcThread, &QThread::finished, calcThread, &QThread::deleteLater);
}

void iAInSpectrAttachment::magicLensToggled( bool /*isOn*/ )
{
	if (dlgXRF && !m_child->channelData(m_xrfChannelID))
	{
		initSlicerXRF( false );
		return;
	}
}

void iAInSpectrAttachment::ioFinished()
{
	ioThread = nullptr;
}
