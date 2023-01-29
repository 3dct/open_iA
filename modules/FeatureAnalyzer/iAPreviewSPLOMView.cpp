// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPreviewSPLOMView.h"

#include "iAPreviewSPLOM.h"
#include "FeatureAnalyzerHelpers.h"

#include <QPixmap>
#include <QDir>

iAPreviewSPLOMView::iAPreviewSPLOMView(QWidget* parent) :
	PreviewSPLOMConnector(parent),
	m_preview( new iAPreviewSPLOM( this ) ),
	m_slicePxmp( new QPixmap ),
	m_datasetsLoaded( false )
{
	QHBoxLayout *layout = new QHBoxLayout( this );
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing( 0 );
	layout->addWidget( m_preview );
	sliceWidget->setLayout( layout );

	spinBoxSliceNumber->setMinimum( 0 );

	connect(m_preview, &iAPreviewSPLOM::roiChanged, this, &iAPreviewSPLOMView::ROIChangedSlot);
	connect(m_preview, &iAPreviewSPLOM::sliceCountsChanged, this, &iAPreviewSPLOMView::sliceCountsChanged);
	connect(spinBoxSliceNumber, QOverload<int>::of(&QSpinBox::valueChanged), this, &iAPreviewSPLOMView::SetSlice);
	connect(sliceScrollBar, &QScrollBar::valueChanged, this, &iAPreviewSPLOMView::SetSlice);
	connect(cbDatasets, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAPreviewSPLOMView::DatasetChanged);
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
	if (m_datasets.isEmpty())
	{
		return;
	}
	for (const QString & d: m_datasets)
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
	for (const QString& d : m_datasets)
	{
		cbDatasets->addItem(d);
	}
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
	if (!m_datasetsLoaded)
	{
		return;
	}
	int dsInd = cbDatasets->currentIndex();
	QString datasetFolder = m_datasetsDir + "/" + m_datasets[dsInd];
	SetSliceCount( m_sliceCntLst[dsInd] );
	SetSlice( m_sliceNumberLst[dsInd] );
	SliceNumberChanged();
	UpdatePixmap();
}

void iAPreviewSPLOMView::SetSlice( int slice )
{
	if (!m_datasetsLoaded)
	{
		return;
	}
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
