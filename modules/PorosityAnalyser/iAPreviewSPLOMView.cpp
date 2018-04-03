/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAPreviewSPLOMView.h"

#include "iAPreviewSPLOM.h"
#include "PorosityAnalyserHelpers.h"

#include <QPixmap>
#include <QDir>

iAPreviewSPLOMView::iAPreviewSPLOMView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) :
	PreviewSPLOMConnector( parent, f ),
	m_preview( new iAPreviewSPLOM( this ) ),
	m_slicePxmp( new QPixmap ),
	m_datasetsLoaded( false )
{
	QHBoxLayout *layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );
	layout->addWidget( m_preview );
	sliceWidget->setLayout( layout );

	spinBoxSliceNumber->setMinimum( 0 );

	connect( m_preview, SIGNAL( roiChanged( QRectF ) ), this, SLOT( ROIChangedSlot( QRectF ) ) );
	connect( m_preview, SIGNAL( sliceCountsChanged( QList<int> ) ), this, SIGNAL( sliceCountsChanged( QList<int> ) ) );
	connect( spinBoxSliceNumber, SIGNAL( valueChanged( int ) ), this, SLOT( SetSlice( int ) ) );
	connect( sliceScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( SetSlice( int ) ) );
	connect( cbDatasets, SIGNAL( currentIndexChanged( int ) ), this, SLOT( DatasetChanged() ) );
}

void iAPreviewSPLOMView::SetDatasetsDir( const QString & datasetsFolder )
{
	m_datasetsDir = datasetsFolder;
}

iAPreviewSPLOMView::~iAPreviewSPLOMView()
{}

void iAPreviewSPLOMView::SetDatasets( QStringList datasets )
{
	m_datasets = datasets;
	LoadDatasets();
}

void iAPreviewSPLOMView::SliceNumberChanged()
{
	int dsInd = cbDatasets->currentIndex();
	spinBoxSliceNumber->setValue( m_sliceNumberLst[dsInd] );
	if( sliceScrollBar->value() != m_sliceNumberLst[dsInd] )
		sliceScrollBar->setValue( m_sliceNumberLst[dsInd] );
	UpdatePixmap();
}


void iAPreviewSPLOMView::SetSliceCount( int sliceCnt )
{
	QSignalBlocker blockSliceScrollBarSignals(sliceScrollBar);
	QSignalBlocker blockSpinBoxSliceNumberSignals(spinBoxSliceNumber);
	sliceScrollBar->setMaximum( sliceCnt - 1 );
	spinBoxSliceNumber->setMaximum( sliceCnt - 1 );
}

void iAPreviewSPLOMView::LoadDatasets()
{
	clear();
	//parse some info
	if( m_datasets.isEmpty() )
		return;
	foreach( const QString & d, m_datasets )
	{
		QString datasetFolder = m_datasetsDir + "/" + d;
		QString fileName = getSliceFilename( datasetFolder, 0 );
		QPixmap pxmp; pxmp.load( fileName );
		m_roiList.push_back( pxmp.rect() );
		double sliceCnt = QDir( getMaskSliceDirNameAbsolute( datasetFolder ) ).count() - 2;
		m_sliceCntLst.push_back( sliceCnt );
		m_sliceNumberLst.push_back( sliceCnt * 0.5 );
	}
	emit roiChanged( m_roiList );
	emit sliceCountsChanged( m_sliceCntLst );
	emit sliceNumbersChanged( m_sliceNumberLst );

	QSignalBlocker blockcbDatasetsSignals(cbDatasets);
	foreach( const QString & d, m_datasets )
		cbDatasets->addItem( d );
	UpdatePixmap();

	m_preview->ResetROI();
	m_datasetsLoaded = true;
	DatasetChanged();
}

void iAPreviewSPLOMView::clear()
{
	m_roiList.clear();
	m_sliceCntLst.clear();
	m_sliceNumberLst.clear();
	QSignalBlocker blockcbDatasetsSignals(cbDatasets);
	cbDatasets->clear();
	m_preview->SetPixmap( 0 );
}

void iAPreviewSPLOMView::UpdatePixmap()
{
	if( !cbDatasets->count() )
		return;
	int dsInd = cbDatasets->currentIndex();
	QString datasetFolder = m_datasetsDir + "/" + m_datasets[dsInd];
	QString fileName = getSliceFilename( datasetFolder, m_sliceNumberLst[dsInd] );
	m_slicePxmp->load( fileName );
	m_preview->SetPixmap( m_slicePxmp.data() );
	if( !m_roiList.empty() )
	{
		m_preview->SetROI( m_roiList[dsInd] );
		emit roiChanged( m_roiList );
	}
}

void iAPreviewSPLOMView::DatasetChanged()
{
	if( !m_datasetsLoaded )
		return;
	int dsInd = cbDatasets->currentIndex();
	QString datasetFolder = m_datasetsDir + "/" + m_datasets[dsInd];
	SetSliceCount( m_sliceCntLst[dsInd] );
	SetSlice( m_sliceNumberLst[dsInd] );
	SliceNumberChanged();
	UpdatePixmap();
}

void iAPreviewSPLOMView::SetSlice( int slice )
{
	if( !m_datasetsLoaded )
		return;
	int dsInd = cbDatasets->currentIndex();
	m_sliceNumberLst[dsInd] = slice;
	SliceNumberChanged();
	emit sliceNumbersChanged( m_sliceNumberLst );
}

void iAPreviewSPLOMView::SetMask( const QPixmap * mask, int datasetIndex )
{
	int dsInd = cbDatasets->currentIndex();
	if( datasetIndex == -1 )
		m_preview->SetMask( mask );
	else if( dsInd == datasetIndex )
		m_preview->SetMask( mask );
	else
		m_preview->SetMask( 0 );
}

void iAPreviewSPLOMView::ROIChangedSlot( QRectF roi )
{
	if( !m_datasetsLoaded )
		return;
	if( !m_roiList.empty() )
	{
		int dsInd = cbDatasets->currentIndex();
		m_roiList[dsInd] = roi;
	}
	emit roiChanged( m_roiList );
}
