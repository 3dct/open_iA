/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "dlg_RefSpectra.h"
#include "dlg_SimilarityMap.h"
#include "dlg_InSpectr.h"
#include "iAElementConcentrations.h"
#include "iAPeriodicTableWidget.h"
#include "iAXRFData.h"

#include <iAChannelData.h>
#include <iADockWidgetWrapper.h>
#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iASlicer.h>
#include <iAToolsVTK.h>
#include <io/iAIO.h>

#include "defines.h"    // for NotExistingChannel

#include <itkMacro.h>    // for itk::ExceptionObject

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QtMath>

iAInSpectrAttachment::iAInSpectrAttachment( iAMainWindow * mainWnd, iAMdiChild * child ) : iAModuleAttachmentToChild( mainWnd, child ),
	dlgPeriodicTable(nullptr),
	dlgSimilarityMap(nullptr),
	dlgXRF(nullptr),
	ioThread(nullptr),
	m_xrfChannelID(NotExistingChannel)
{
	connect(m_child, &iAMdiChild::magicLensToggled, this, &iAInSpectrAttachment::magicLensToggled);
	for (int i = 0; i < 3; ++i)
	{
		connect(m_child->slicer(i), &iASlicer::mouseMoved, this, &iAInSpectrAttachment::updateXRFVoxelEnergy);
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

	LOG(lvlInfo, tr("Loading file '%1', please wait...").arg(f));

	auto periodicTable = new iAPeriodicTableWidget(m_child);
	dlgPeriodicTable = new iADockWidgetWrapper(periodicTable, "Periodic Table of Elements", "PeriodicTable");
	dlgRefSpectra = new dlg_RefSpectra( m_child );
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), dlgPeriodicTable, Qt::Horizontal);
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), dlgRefSpectra, Qt::Horizontal);

	dlgXRF = new dlg_InSpectr(m_child, periodicTable, dlgRefSpectra);

	ioThread = new iAIO(iALog::get(), m_child, dlgXRF->GetXRFData()->GetDataPtr() );
	m_child->setReInitializeRenderWindows( false );
	m_child->connectIOThreadSignals( ioThread );
	connect( ioThread, &iAIO::done, this, &iAInSpectrAttachment::xrfLoadingDone);
	connect( ioThread, &iAIO::failed, this, &iAInSpectrAttachment::xrfLoadingFailed);
	connect( ioThread, &iAIO::finished, this, &iAInSpectrAttachment::ioFinished);

	QString extension = QFileInfo( f ).suffix();
	extension = extension.toUpper();
	if (extensionToIdStack().find(extension) == extensionToIdStack().end())
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "Unsupported extension");
	}

	iAIOType id = extensionToIdStack().find(extension).value();
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
	LOG(lvlInfo, tr("Spectral color image initialized."));
}

QThread* iAInSpectrAttachment::recalculateXRF()
{
	if( !dlgXRF )
	{
		return nullptr;
	}
	return dlgXRF->UpdateForVisualization();
}

void iAInSpectrAttachment::updateXRFVoxelEnergy( double x, double y, double z, int /*mode*/ )
{
	if (!dlgXRF || !dlgXRF->cb_spectrumProbing->isChecked())
	{
		return;
	}
	iAXRFData * xrfData = dlgXRF->GetXRFData().data();
	if (!xrfData || xrfData->begin() == xrfData->end())
	{
		return;
	}
	auto firstImg = *xrfData->begin();
	double worldCoord[3] = { x, y, z };
	auto xrfCoord = mapWorldCoordsToIndex(firstImg, worldCoord);
	dlgXRF->UpdateVoxelSpectrum(xrfCoord.x(), xrfCoord.y(), xrfCoord.z());

	if( dlgXRF->isDecompositionLoaded() )
	{
		auto concImg = dlgXRF->GetElementConcentrations()->getImage(0);
		auto concPos = mapWorldCoordsToIndex(concImg, worldCoord);
		dlgXRF->UpdateConcentrationViews( concPos.x(), concPos.y(), concPos.z());
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
	dlgXRF->init(minEnergy, maxEnergy, haveEnergyLevels, m_child);
	connect( dlgXRF->cb_spectralColorImage, &QCheckBox::stateChanged, this, &iAInSpectrAttachment::visualizeXRF);
	connect( dlgXRF->sl_peakOpacity, &QSlider::valueChanged, this, &iAInSpectrAttachment::updateXRFOpacity);
	connect( dlgXRF->pb_compute, &QPushButton::clicked, this, &iAInSpectrAttachment::updateXRF);
	m_child->splitDockWidget(dlgRefSpectra, dlgXRF, Qt::Horizontal);
	dlgSimilarityMap->connectToXRF( dlgXRF );
	m_child->updateLayout();
}

void iAInSpectrAttachment::xrfLoadingFailed()
{
	LOG(lvlError, tr("XRF data loading has failed!"));
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
	m_child->tabifyDockWidget( m_child->renderDockWidget(), dlgSimilarityMap );

	return true;
}

void iAInSpectrAttachment::initSlicerXRF( bool enableChannel )
{
	assert( !m_child->channelData(m_xrfChannelID) );
	LOG(lvlInfo, tr("Initializing Spectral Color Image. This may take a while..."));
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
