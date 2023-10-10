// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFuzzyFeatureTrackingTool.h"

#include "dlg_dataView4DCT.h"
#include "dlg_eventExplorer.h"
#include "dlg_trackingGraph.h"
#include "iAFeatureTracking.h"

#include <iADockWidgetWrapper.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAVolumeViewer.h>
#include <iAMdiChild.h>

#include <itkMacro.h>    // for itk::ExceptionObject

#include <QFile>

const int FOURDCT_MIN_NUMBER_OF_VOLUMES = 2;

iAFuzzyFeatureTrackingTool::iAFuzzyFeatureTrackingTool( iAMainWindow * mainWnd, iAMdiChild * child ):
	iATool( mainWnd, child ),
	m_dlgDataView4DCT(nullptr),
	m_dlgTrackingGraph(nullptr),
	m_dlgEventExplorer(nullptr)
{
	connect( child, &iAMdiChild::viewsUpdated, this, &iAFuzzyFeatureTrackingTool::updateViews);
	std::vector<iAVolumeViewer*> volumeViewers;
	for (auto ds : child->dataSetMap())
	{
		if (ds.second->type() == iADataSetType::Volume)
		{
			volumeViewers.push_back(dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(ds.first)));
		}
	}
	if (volumeViewers.size() < FOURDCT_MIN_NUMBER_OF_VOLUMES)
	{
		throw std::runtime_error(QString( "No volume stack loaded or it does not contain enough volumes (expected: %1, actual: %2)!" )
			.arg(FOURDCT_MIN_NUMBER_OF_VOLUMES)
			.arg(volumeViewers.size()).toStdString() );
	}

	m_dlgDataView4DCT = new dlg_dataView4DCT(m_child, volumeViewers);
	m_child->tabifyDockWidget(m_child->renderDockWidget(), new iADockWidgetWrapper(m_dlgDataView4DCT, "4DCT Data View", "DataView4DCT"));
	m_dlgTrackingGraph = new dlg_trackingGraph(m_child);
	m_child->tabifyDockWidget(m_child->renderDockWidget(), m_dlgTrackingGraph);
	std::vector<iAFeatureTracking*> trackedFeaturesForwards;
	std::vector<iAFeatureTracking*> trackedFeaturesBackwards;

	for (size_t i = 0; i < volumeViewers.size(); ++i)
	{
		QString file[2];

		const QString csvExt = QString( ".csv" );

		if (i == 0)
		{
			file[0] = QString();
		}
		else
		{
			file[0] = volumeViewers[i-1]->volume()->metaData(iADataSet::FileNameKey).toString() + csvExt;
		}
		file[1] = volumeViewers[i]->volume()->metaData(iADataSet::FileNameKey).toString() + csvExt;

		if (!file[0].isEmpty() && !QFile::exists(file[0]))
		{
			throw std::runtime_error(QString("The file \"%1\" is missing").arg(file[0]).toStdString());
		}
		if (!QFile::exists(file[1]))
		{
			throw std::runtime_error(QString( "The file \"%1\" is missing").arg(file[1]).toStdString());
		}

		int lastIndex = file[1].lastIndexOf( QString( "/" ) );
		QString outputFileName = file[1].left( lastIndex + 1 ) + "outF" + QString::number(i) + ".txt";

		const float	dissipationThreshold = 0.02f;
		const float	overlapThreshold = 0.90f;
		const float	volumeThreshold = 0.05f;
		const int	maxSearchValue = 0;
		const int	lineOffset = 4;

		trackedFeaturesForwards.push_back( new iAFeatureTracking( file[0], file[1], lineOffset, outputFileName, dissipationThreshold, overlapThreshold, volumeThreshold, maxSearchValue ) );
		if (i == 0)
		{
			trackedFeaturesBackwards.push_back(new iAFeatureTracking(file[0], file[1], lineOffset, outputFileName, dissipationThreshold, overlapThreshold, volumeThreshold, maxSearchValue));
		}
		else
		{
			trackedFeaturesBackwards.push_back(new iAFeatureTracking(file[1], file[0], lineOffset, outputFileName, dissipationThreshold, overlapThreshold, volumeThreshold, maxSearchValue));
		}
	}
	for (size_t i = 0; i < trackedFeaturesForwards.size(); ++i)
	{
		trackedFeaturesForwards.at(i)->TrackFeatures();
		trackedFeaturesBackwards.at(i)->TrackFeatures();
	}
	m_dlgEventExplorer = new dlg_eventExplorer( m_child, trackedFeaturesForwards.size(), 5, volumeViewers, m_dlgTrackingGraph, trackedFeaturesForwards, trackedFeaturesBackwards );
	m_child->tabifyDockWidget( m_child->renderDockWidget(), m_dlgEventExplorer );
}

void iAFuzzyFeatureTrackingTool::updateViews()
{
	if (m_dlgDataView4DCT)
	{
		m_dlgDataView4DCT->update();
	}
}
