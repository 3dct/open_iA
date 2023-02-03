// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFuzzyFeatureTrackingTool.h"

#include "dlg_dataView4DCT.h"
#include "dlg_eventExplorer.h"
#include "dlg_trackingGraph.h"
#include "iAFeatureTracking.h"

#include <iADockWidgetWrapper.h>
#include <iALog.h>
#include <iAVolumeStack.h>
#include <iAMdiChild.h>

#include <itkMacro.h>    // for itk::ExceptionObject

#include <QFile>

const int FOURDCT_MIN_NUMBER_OF_VOLUMES = 2;

iAFuzzyFeatureTrackingTool::iAFuzzyFeatureTrackingTool( iAMainWindow * mainWnd, iAMdiChild * child ):
	iATool( mainWnd, child ),
	m_dlgDataView4DCT(nullptr),
	m_dlgTrackingGraph(nullptr),
	m_dlgEventExplorer(nullptr),
	m_volumeStack(child->volumeStack())
{
	connect( child, &iAMdiChild::viewsUpdated, this, &iAFuzzyFeatureTrackingTool::updateViews);

	if (!create4DCTDataViewWidget())
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "create4DCTDataViewWidget failed");
	}
	if( !create4DCTTrackingGraphWidget() )
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "create4DCTTrackingGraphWidget failed");
	}
	if( !create4DCTEventExplorerWidget() )
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "create4DCTEventExplorerWidget failed");
	}
}

bool iAFuzzyFeatureTrackingTool::create4DCTDataViewWidget()
{
	if (m_dlgDataView4DCT)
	{
		LOG(lvlWarn, tr( "The data view 4DCT dialog already exists!" ) );
		return false;
	}

	if (!m_volumeStack || m_volumeStack->numberOfVolumes() < FOURDCT_MIN_NUMBER_OF_VOLUMES)
	{
		LOG(lvlError, tr( "No volume stack loaded or it does not contain enough volumes (expected: %1, actual: %2)!" )
			.arg(FOURDCT_MIN_NUMBER_OF_VOLUMES)
			.arg(m_volumeStack->numberOfVolumes()) );
		return false;
	}

	m_dlgDataView4DCT = new dlg_dataView4DCT( m_child, m_volumeStack );
	m_child->tabifyDockWidget( m_child->renderDockWidget(), new iADockWidgetWrapper(m_dlgDataView4DCT, "4DCT Data View", "DataView4DCT") );
	// test m_renderer->reInitialize (m_volumeStack->volume(1), polyData, m_volumeStack->opacityTF(1),m_volumeStack->colorTF(1));

	LOG(lvlInfo, tr( "The 4DCT Data View widget was successfully created" ) );

	return true;
}

bool iAFuzzyFeatureTrackingTool::create4DCTTrackingGraphWidget()
{
	if (m_dlgTrackingGraph)
	{
		LOG(lvlWarn, tr( "The Tracking Graph widget already exists!" ) );
		return false;
	}

	m_dlgTrackingGraph = new dlg_trackingGraph( m_child );
	m_child->tabifyDockWidget( m_child->renderDockWidget(), m_dlgTrackingGraph );

	LOG(lvlInfo, tr( "The Tracking Graph widget was successfully created" ) );

	return true;
}

bool iAFuzzyFeatureTrackingTool::create4DCTEventExplorerWidget()
{
	if (m_dlgEventExplorer)
	{
		LOG(lvlWarn, tr( "The Event Explorer widget already exists!" ) );
		return false;
	}

	if (!m_dlgTrackingGraph)
	{
		LOG(lvlError, tr( "The Tracking Graph widget is missing. It is required for creating the Event Explorer widget" ) );
		return false;
	}

	std::vector<iAFeatureTracking*> trackedFeaturesForwards;
	std::vector<iAFeatureTracking*> trackedFeaturesBackwards;

	for (size_t i = 0; i < m_volumeStack->fileNames()->size(); ++i)
	{
		QString file[2];

		const QString csvExt = QString( ".csv" );

		if (i == 0)
		{
			file[0] = QString();
		}
		else
		{
			file[0] = m_volumeStack->fileName( i - 1 ) + csvExt;
		}
		file[1] = m_volumeStack->fileName( i ) + csvExt;

		if (!file[0].isEmpty() && !QFile::exists(file[0]))
		{
			LOG(lvlError, QString( "The file \"" ) + file[0] + QString( "\" is missing" ) );
			return false;
		}
		if (!QFile::exists(file[1]))
		{
			LOG(lvlError, QString( "The file \"" ) + file[1] + QString( "\" is missing" ) );
			return false;
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

	// 		AllocConsole();
	// 		freopen("CON", "w", stdout);

	for (size_t i = 0; i < trackedFeaturesForwards.size(); ++i)
	{
		trackedFeaturesForwards.at(i)->TrackFeatures();
		trackedFeaturesBackwards.at(i)->TrackFeatures();
	}

	m_dlgEventExplorer = new dlg_eventExplorer( m_child, trackedFeaturesForwards.size(), 5, m_volumeStack, m_dlgTrackingGraph, trackedFeaturesForwards, trackedFeaturesBackwards );
	m_child->tabifyDockWidget( m_child->renderDockWidget(), m_dlgEventExplorer );
	LOG(lvlInfo, tr( "The Event Explorer widget was successfully created" ) );

	return true;
}

iAFuzzyFeatureTrackingTool::~iAFuzzyFeatureTrackingTool()
{
	delete m_dlgDataView4DCT;
}

void iAFuzzyFeatureTrackingTool::updateViews()
{
	if (m_dlgDataView4DCT)
	{
		m_dlgDataView4DCT->update();
	}
}
