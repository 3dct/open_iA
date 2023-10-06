// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAInSpectrTool.h"

#include "dlg_RefSpectra.h"
#include "dlg_SimilarityMap.h"
#include "dlg_InSpectr.h"
#include "iAElementConcentrations.h"
#include "iAPeriodicTableWidget.h"
#include "iAXRFData.h"

#include <iAChannelData.h>
#include <iADockWidgetWrapper.h>
//#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iARunAsync.h>
#include <iASlicer.h>
#include <iAToolsVTK.h>

#include <iAFileTypeRegistry.h>

#include <iALog.h>
#include <defines.h>    // for NotExistingChannel

#include <itkMacro.h>    // for itk::ExceptionObject

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QThread>
#include <QtMath>

const QString iAInSpectrTool::Name("InSpectr");

iAInSpectrTool::iAInSpectrTool( iAMainWindow * mainWnd, iAMdiChild * child ) : iATool( mainWnd, child ),
	dlgPeriodicTable(nullptr),
	dlgSimilarityMap(nullptr),
	dlgXRF(nullptr),
	m_xrfChannelID(NotExistingChannel)
{
	connect(m_child, &iAMdiChild::magicLensToggled, this, &iAInSpectrTool::magicLensToggled);
	for (int i = 0; i < 3; ++i)
	{
		connect(m_child->slicer(i), &iASlicer::mouseMoved, this, &iAInSpectrTool::updateXRFVoxelEnergy);
	}
	//TODO: move
	if (!filter_SimilarityMap())
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "filter_SimilarityMap failed");
	}
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr( "Open File" ),
		m_child->filePath(),
		tr( "All supported types (*.mhd *.raw *.volstack);;MetaImages (*.mhd *.mha);;RAW files (*.raw);;Volume Stack (*.volstack)" ) );
	if (!QFile::exists(fileName))
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "File does not exist");
	}

	LOG(lvlInfo, tr("Loading file '%1', please wait...").arg(fileName));

	auto periodicTable = new iAPeriodicTableWidget(m_child);
	dlgPeriodicTable = new iADockWidgetWrapper(periodicTable, "Periodic Table of Elements", "PeriodicTable");
	dlgRefSpectra = new dlg_RefSpectra( m_child );
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), dlgPeriodicTable, Qt::Horizontal);
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), dlgRefSpectra, Qt::Horizontal);

	dlgXRF = new dlg_InSpectr(m_child, periodicTable, dlgRefSpectra);

	auto io = iAFileTypeRegistry::createIO(fileName, iAFileIO::Load);
	if (!io)
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "Unsupported extension");
	}

	auto energyRangeStr = std::make_shared<QString>();
	runAsync(
		[this, io, fileName, energyRangeStr]()
		{
			auto collection = std::dynamic_pointer_cast<iADataCollection>(io->load(fileName, QVariantMap()));
			if (!collection)
			{
				return;
			}
			for (auto ds : collection->dataSets())
			{
				auto imgDS = dynamic_cast<iAImageData*>(ds.get());
				dlgXRF->GetXRFData()->GetDataContainer().push_back(imgDS->vtkImage());
			}
			*energyRangeStr = collection->metaData("energy_range").toString();
		},
		[this, energyRangeStr]() {
			/* xrfLoadingDone() */
			if (dlgXRF->GetXRFData()->GetDataContainer().empty())
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
				m_child->removeTool(Name);
			} else {
				double minEnergy = 0;
				double maxEnergy = dlgXRF->GetXRFData()->size();
				bool haveEnergyLevels = false;
				QString energyRange = *energyRangeStr;
				if (!energyRange.isEmpty())
				{
					QStringList energies = energyRange.split(":");
					if (energies.size() == 2)
					{
						minEnergy = energies[0].toDouble();
						maxEnergy = energies[1].toDouble();
						haveEnergyLevels = true;
					}
				}
				dlgXRF->init(minEnergy, maxEnergy, haveEnergyLevels, m_child);
				connect(dlgXRF->cb_spectralColorImage, &QCheckBox::stateChanged, this, &iAInSpectrTool::visualizeXRF);
				connect(dlgXRF->sl_peakOpacity, &QSlider::valueChanged, this, &iAInSpectrTool::updateXRFOpacity);
				connect(dlgXRF->pb_compute, &QPushButton::clicked, this, &iAInSpectrTool::updateXRF);
				m_child->splitDockWidget(dlgRefSpectra, dlgXRF, Qt::Horizontal);
				dlgSimilarityMap->connectToXRF(dlgXRF);
				m_child->updateLayout();
			}
		}, this
	);
}

void iAInSpectrTool::reInitXRF()
{
	vtkSmartPointer<vtkImageData> img = dlgXRF->GetCombinedVolume();
	//if (m_child->isMagicLens2DEnabled())
	//{
	//	for (int s = 0; s < 3; ++s)
	//	{
	//		m_child->slicer(s)->updateChannel(m_xrfChannelID, iAChannelData("Spectral Color Image", img, dlgXRF->GetColorTransferFunction()));
	//	}
	//	m_child->setMagicLensInput(m_xrfChannelID);
	//}
	if (m_child->channelData(m_xrfChannelID) && m_child->channelData(m_xrfChannelID)->isEnabled())
	{
		m_child->updateChannel(m_xrfChannelID, img, dlgXRF->GetColorTransferFunction(), nullptr, false);
	}
	// TODO: NewIO Test!
	if (m_child->isMagicLens2DEnabled())
	{
		m_child->setMagicLensInput(m_xrfChannelID);
	}
}

void iAInSpectrTool::initXRF()
{
	initXRF( true );
}

void iAInSpectrTool::deinitXRF()
{
	initXRF( false );
}

void iAInSpectrTool::initXRF( bool enableChannel )
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

QThread* iAInSpectrTool::recalculateXRF()
{
	if( !dlgXRF )
	{
		return nullptr;
	}
	return dlgXRF->UpdateForVisualization();
}

void iAInSpectrTool::updateXRFVoxelEnergy( double x, double y, double z, int /*mode*/ )
{
	if (!dlgXRF || !dlgXRF->cb_spectrumProbing->isChecked())
	{
		return;
	}
	iAXRFData * xrfData = dlgXRF->GetXRFData().get();
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

void iAInSpectrTool::updateSlicerXRFOpacity()
{
	double opacity = (double)dlgXRF->sl_peakOpacity->value() / dlgXRF->sl_peakOpacity->maximum();
	m_child->updateChannelOpacity(m_xrfChannelID, opacity );
}

void iAInSpectrTool::updateXRFOpacity( int /*value*/ )
{
	iAChannelData * data = m_child->channelData(m_xrfChannelID);
	if (data && data->isEnabled())
	{
		dlgXRF->sl_peakOpacity->repaint();
		updateSlicerXRFOpacity();
	}
}

bool iAInSpectrTool::filter_SimilarityMap()
{
	dlgSimilarityMap = new dlg_SimilarityMap( m_child );
	m_child->tabifyDockWidget( m_child->renderDockWidget(), dlgSimilarityMap );

	return true;
}

void iAInSpectrTool::initSlicerXRF( bool enableChannel )
{
	assert( !m_child->channelData(m_xrfChannelID) );
	LOG(lvlInfo, tr("Initializing Spectral Color Image. This may take a while..."));
	auto calcThread = recalculateXRF();
	if (enableChannel)
	{
		QObject::connect(calcThread, &QThread::finished, this, QOverload<>::of(&iAInSpectrTool::initXRF));
	}
	else
	{
		QObject::connect(calcThread, &QThread::finished, this, &iAInSpectrTool::deinitXRF);
	}
}

void iAInSpectrTool::visualizeXRF( int isOn )
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

void iAInSpectrTool::updateXRF()
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
	connect( calcThread, &QThread::finished, this, &iAInSpectrTool::reInitXRF);
	connect( calcThread, &QThread::finished, calcThread, &QThread::deleteLater);
}

void iAInSpectrTool::magicLensToggled( bool /*isOn*/ )
{
	if (dlgXRF && !m_child->channelData(m_xrfChannelID))
	{
		initSlicerXRF( false );
		return;
	}
}
