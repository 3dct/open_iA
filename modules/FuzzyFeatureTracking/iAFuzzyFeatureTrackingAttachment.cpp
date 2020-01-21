/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFuzzyFeatureTrackingAttachment.h"

#include "dlg_dataView4DCT.h"
#include "dlg_eventExplorer.h"
#include "dlg_trackingGraph.h"
#include "iAFeatureTracking.h"

#include <iAVolumeStack.h>
#include <mdichild.h>

#include <itkMacro.h>    // for itk::ExceptionObject

const int FOURDCT_MIN_NUMBER_OF_VOLUMES = 1;

iAFuzzyFeatureTrackingAttachment::iAFuzzyFeatureTrackingAttachment( MainWindow * mainWnd, MdiChild * child ) : iAModuleAttachmentToChild( mainWnd, child ),
	trackingGraph( 0 ), m_dlgDataView4DCT( 0 ), m_dlgTrackingGraph( 0 ), m_dlgEventExplorer( 0 ), m_volumeStack( 0 )
{
	m_volumeStack = child->volumeStack();
	connect( child, SIGNAL( viewsUpdated() ), this, SLOT( updateViews() ) );

	if( !create4DCTDataViewWidget() )
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

bool iAFuzzyFeatureTrackingAttachment::create4DCTDataViewWidget()
{
	if( m_dlgDataView4DCT )
	{
		m_child->addMsg( tr( "The data view 4DCT dialog already exists!" ) );
		return false;
	}

	if( !m_volumeStack || m_volumeStack->numberOfVolumes() <= FOURDCT_MIN_NUMBER_OF_VOLUMES )
	{
		m_child->addMsg( tr( "No volume stack loaded or it does not contain enough volumes (expected: %1, actual: %2)!" )
			.arg(FOURDCT_MIN_NUMBER_OF_VOLUMES)
			.arg(m_volumeStack->numberOfVolumes()) );
		return false;
	}

	// create new dialog
	m_dlgDataView4DCT = new dlg_dataView4DCT( m_child, m_volumeStack );
	m_child->tabifyDockWidget( m_child->logDockWidget(), m_dlgDataView4DCT );
	// test m_renderer->reInitialize (m_volumeStack->volume(1), polyData, m_volumeStack->opacityTF(1),m_volumeStack->colorTF(1));

	m_child->addMsg( tr( "The 4DCT Data View widget was successfully created" ) );

	return true;
}

bool iAFuzzyFeatureTrackingAttachment::create4DCTTrackingGraphWidget()
{
	if( m_dlgTrackingGraph )
	{
		m_child->addMsg( tr( "The Tracking Graph widget already exists!" ) );
		return false;
	}

	m_dlgTrackingGraph = new dlg_trackingGraph( m_child );
	m_child->tabifyDockWidget( m_child->logDockWidget(), m_dlgTrackingGraph );

	m_child->addMsg( tr( "The Tracking Graph widget was successfully created" ) );

	return true;
}

bool iAFuzzyFeatureTrackingAttachment::create4DCTEventExplorerWidget()
{
	if( m_dlgEventExplorer )
	{
		m_child->addMsg( tr( "The Event Explorer widget already exists!" ) );
		return false;
	}

	if( !m_dlgTrackingGraph )
	{
		m_child->addMsg( tr( "The Tracking Graph widget is missing. It is required for creating the Event Explorer widget" ) );
		return false;
	}

	std::vector<iAFeatureTracking*> trackedFeaturesForwards;
	std::vector<iAFeatureTracking*> trackedFeaturesBackwards;

	for( int i = 0; i < m_volumeStack->fileNames()->size(); i++ )
	{
		QString file[2];

		const QString csvExt = QString( ".csv" );

		if( i == 0 ) {
			file[0] = QString();
		}
		else {
			file[0] = m_volumeStack->fileName( i - 1 ) + csvExt;
		}
		file[1] = m_volumeStack->fileName( i ) + csvExt;

		if( !file[0].isEmpty() && !QFile::exists( file[0] ) )
		{
			m_child->addMsg( QString( "The file \"" ) + file[0] + QString( "\" is missing" ) );
			return false;
		}
		if( !QFile::exists( file[1] ) )
		{
			m_child->addMsg( QString( "The file \"" ) + file[1] + QString( "\" is missing" ) );
			return false;
		}

		int lastIndex = file[1].lastIndexOf( QString( "/" ) );
		QString outputFileName = file[1].left( lastIndex + 1 ) + "outF" + i + ".txt";

		const float	dissipationThreshold = 0.02f;
		const float	overlapThreshold = 0.90f;
		const float	volumeThreshold = 0.05f;
		const int	maxSearchValue = 0;
		const int	lineOffset = 4;

		trackedFeaturesForwards.push_back( new iAFeatureTracking( file[0], file[1], lineOffset, outputFileName, dissipationThreshold, overlapThreshold, volumeThreshold, maxSearchValue ) );
		if( i == 0 )
			trackedFeaturesBackwards.push_back( new iAFeatureTracking( file[0], file[1], lineOffset, outputFileName, dissipationThreshold, overlapThreshold, volumeThreshold, maxSearchValue ) );
		else
			trackedFeaturesBackwards.push_back( new iAFeatureTracking( file[1], file[0], lineOffset, outputFileName, dissipationThreshold, overlapThreshold, volumeThreshold, maxSearchValue ) );
	}

	// 		AllocConsole();
	// 		freopen("CON", "w", stdout);

	for( int i = 0; i < trackedFeaturesForwards.size(); i++ )
	{
		trackedFeaturesForwards.at( i )->TrackFeatures();
		trackedFeaturesBackwards.at( i )->TrackFeatures();
	}

	m_dlgEventExplorer = new dlg_eventExplorer( m_child, trackedFeaturesForwards.size(), 5, m_volumeStack, m_dlgTrackingGraph, trackedFeaturesForwards, trackedFeaturesBackwards );
	m_child->tabifyDockWidget( m_child->logDockWidget(), m_dlgEventExplorer );
	m_child->addMsg( tr( "The Event Explorer widget was successfully created" ) );

	return true;
}

iAFuzzyFeatureTrackingAttachment::~iAFuzzyFeatureTrackingAttachment()
{
	if (m_dlgDataView4DCT)	delete m_dlgDataView4DCT;
}

void iAFuzzyFeatureTrackingAttachment::updateViews()
{
	if( m_dlgDataView4DCT )
	{
		m_dlgDataView4DCT->update();
	}
}
