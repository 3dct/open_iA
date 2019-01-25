/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#include "iASSView.h"

#include "iASegm3DView.h"
#include "iASSSlicer.h"
#include "iASSViewSetings.h"
#include "PorosityAnalyserHelpers.h"

#include <iAChannelVisualizationData.h>
#include <iAConsole.h>
#include <defines.h>
#include <iACSVToQTableWidgetConverter.h>
#include <iABoxPlotData.h>
#include <iASlicer.h>
#include <iAChanData.h>
#include <iARenderer.h>
#include <iAVTKRendererManager.h>
#include <io/iAFileUtils.h>

#include <vtkTransform.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageData.h>
#include <vtkMarchingContourFilter.h>
#include <vtkDistancePolyDataFilter.h>

#include <QSettings>

const iASlicerMode mode[3] = { XY, YZ, XZ };
const int extentIndices[3][2] = { { 4, 5 }, { 0, 1 }, { 2, 3 } };

extern const iAChannelID MasksChanID;
extern const iAChannelID GTChanID;
extern const iAChannelID minChanID;
extern const iAChannelID medChanID;
extern const iAChannelID maxChanID;

inline double NormalizedSliderValue(QSlider * slider)
{
	return (double)slider->value() / ( slider->maximum() - slider->minimum() );
}

void loadImageData( QString const & fileName, vtkSmartPointer<vtkImageData> & imgData );

iASSView::iASSView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: PorosityAnalyzerSSConnector( parent, f ),
	m_SSViewSettings( new iASSViewSettings( this, f ) ),
	m_slicerTransform( vtkSmartPointer<vtkTransform>::New() ),
	m_slicerTF( vtkSmartPointer<vtkColorTransferFunction>::New() ),
	m_modeInd( 0 ),
	m_sliceMgr( new iAVTKRendererManager ),
	m_imgData( vtkSmartPointer<vtkImageData>::New() ),
	m_slicerViewsLayout( new QHBoxLayout(slicerWidget) ),
	m_segm3DViewExtrnl( 0 ),
	m_runsOffset( -10000 )
{
	m_slicerViewsLayout->setMargin( 0 );
	QSettings settings( organisationName, applicationName );
	m_datasetFolder = settings.value( "PorosityAnalyser/GUI/datasetsFolder", "" ).toString();
	m_resultsFolder = settings.value( "PorosityAnalyser/GUI/resultsFolder", "" ).toString();
	m_SSViewSettings->cbShowMasks->setChecked( settings.value( "PorosityAnalyser/GUI/ShowMasks", false ).toBool() );
	m_SSViewSettings->sMasksOpacity->setValue( settings.value( "PorosityAnalyser/GUI/MasksOpacity", 0 ).toInt() );
	m_SSViewSettings->cbShowGT->setChecked( settings.value( "PorosityAnalyser/GUI/ShowGT", false ).toBool() );
	m_SSViewSettings->sGTOpacity->setValue( settings.value( "PorosityAnalyser/GUI/GTOpacity", 0 ).toInt() );
	m_SSViewSettings->cbShowContours->setChecked( settings.value( "PorosityAnalyser/GUI/ShowContours", false ).toBool() );

	m_SSViewSettings->cbShowVolume->setChecked( settings.value( "PorosityAnalyser/GUI/ShowVolume", false ).toBool() );
	m_SSViewSettings->cbShowSurface->setChecked( settings.value( "PorosityAnalyser/GUI/ShowSurface", false ).toBool() );
	m_SSViewSettings->cbShowWireframe->setChecked( settings.value( "PorosityAnalyser/GUI/ShowWireframe", false ).toBool() );
	m_SSViewSettings->cbDeviation->setCurrentIndex( settings.value( "PorosityAnalyser/GUI/Deviation", 0 ).toInt() );
	m_deviationMode = m_SSViewSettings->cbDeviation->currentIndex();

	connect( sbNum, SIGNAL( valueChanged( int ) ), this, SLOT( setSliceSpinBox( int ) ) );
	connect( verticalScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( setSliceScrollBar( int ) ) );
	connect( cbDir, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setSlicerDirection( int ) ) );
	connect( m_SSViewSettings->cbShowMasks, SIGNAL( stateChanged( int ) ), this, SLOT( setShowMasks( int ) ) );
	connect( m_SSViewSettings->sMasksOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( setMasksOpacity( int ) ) );
	connect( m_SSViewSettings->cbShowGT, SIGNAL( stateChanged( int ) ), this, SLOT( setShowGT( int ) ) );
	connect( m_SSViewSettings->sGTOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( setGTOpacity( int ) ) );
	connect( m_SSViewSettings->cbShowContours, SIGNAL( stateChanged( int ) ), this, SLOT( setShowContours( int ) ) );
	connect( m_SSViewSettings->cbShowVolume, SIGNAL( stateChanged( int ) ), this, SLOT( setShowVolume( int ) ) );
	connect( m_SSViewSettings->cbShowSurface, SIGNAL( stateChanged( int ) ), this, SLOT( setShowSurface( int ) ) );
	connect( m_SSViewSettings->cbShowWireframe, SIGNAL( stateChanged( int ) ), this, SLOT( setShowWireframe( int ) ) );
	connect( m_SSViewSettings->cbDeviation, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setDeviationMode( int ) ) );
	connect( m_SSViewSettings->sSensitivity, SIGNAL( valueChanged( int ) ), this, SLOT( setSensitivity( int ) ) );

	connect( m_SSViewSettings->cbShowMasks, SIGNAL( stateChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->sMasksOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->cbShowGT, SIGNAL( stateChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->sGTOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->cbShowContours, SIGNAL( stateChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->cbShowVolume, SIGNAL( stateChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->cbShowSurface, SIGNAL( stateChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->cbShowWireframe, SIGNAL( stateChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->cbDeviation, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateSettings() ) );
	connect( m_SSViewSettings->sSensitivity, SIGNAL( valueChanged( int ) ), this, SLOT( updateSettings() ) );
	
	connect( tbSettings, SIGNAL( clicked() ), this, SLOT( showSettings() ) );
}

iASSView::~iASSView()
{
	foreach( iASSSlicer * s, m_slicerViews )
		delete s;
	updateSettings();
}

void iASSView::updateSettings()
{
	QSettings settings( organisationName, applicationName );
	settings.setValue( "PorosityAnalyser/GUI/ShowMasks", m_SSViewSettings->cbShowMasks->isChecked() );
	settings.setValue( "PorosityAnalyser/GUI/MasksOpacity", m_SSViewSettings->sMasksOpacity->value() );
	settings.setValue( "PorosityAnalyser/GUI/ShowGT", m_SSViewSettings->cbShowGT->isChecked() );
	settings.setValue( "PorosityAnalyser/GUI/GTOpacity", m_SSViewSettings->sGTOpacity->value() );
	settings.setValue( "PorosityAnalyser/GUI/ShowContours", m_SSViewSettings->cbShowContours->isChecked() );

	settings.setValue( "PorosityAnalyser/GUI/ShowVolume", m_SSViewSettings->cbShowVolume->isChecked() );
	settings.setValue( "PorosityAnalyser/GUI/ShowSurface", m_SSViewSettings->cbShowSurface->isChecked() );
	settings.setValue( "PorosityAnalyser/GUI/ShowWireframe", m_SSViewSettings->cbShowWireframe->isChecked() );
	settings.setValue( "PorosityAnalyser/GUI/Deviation", m_SSViewSettings->cbDeviation->currentIndex() );
}

void iASSView::BuildDefaultTF( vtkSmartPointer<vtkImageData> & imgData, vtkSmartPointer<vtkColorTransferFunction> & tf, QColor color )
{
	if (!imgData)
		DEBUG_LOG("Image data is NULL!");
	tf->RemoveAllPoints();
	tf->AddRGBPoint( imgData->GetScalarRange()[0], 0.0, 0.0, 0.0 );
	tf->AddRGBPoint( imgData->GetScalarRange()[1], color.redF(), color.greenF(), color.blueF() );
	tf->Build();
}

void iASSView::InitializeGUI()
{
	int extent[2] = { m_imgData->GetExtent()[ extentIndices[m_modeInd][0] ], m_imgData->GetExtent()[ extentIndices[m_modeInd][1] ] };
	sbNum->setRange( extent[0], extent[1] );
	verticalScrollBar->setRange( extent[0], extent[1] );
	int val = (extent[1] - extent[0]) * 0.5 + extent[0];
	sbNum->setValue( val );
	verticalScrollBar->setValue( val );

	setSliceSpinBox( sbNum->value() );
	setShowMasks( m_SSViewSettings->cbShowMasks->isChecked() );
	setMasksOpacity( m_SSViewSettings->sMasksOpacity->value() );
	setShowGT( m_SSViewSettings->cbShowGT->isChecked() );
	setGTOpacity( m_SSViewSettings->sGTOpacity->value() );
	setShowContours( m_SSViewSettings->cbShowContours->isChecked() );
}

void iASSView::LoadDataToSlicer( iASSSlicer * slicer, const QTableWidget * data )
{
	//data itself
	m_datasetFile = m_datasetFolder + "/" + data->item( 0, datasetColInd )->text();
	loadImageData( m_datasetFile, m_imgData );
	BuildDefaultTF( m_imgData, m_slicerTF );
	slicer->initialize( m_imgData, m_slicerTransform, m_slicerTF );

	//masks channel
	QStringList masks;
	for( int i = 0; i < data->rowCount(); ++i )
		masks << data->item( i, m_runsOffset + maskOffsetInRuns )->text();

	slicer->initializeMasks( masks );

	//gt channel
	QTableWidget dsDescr;
	iACSVToQTableWidgetConverter::loadCSVFile( m_resultsFolder + "/DatasetDescription.csv", &dsDescr );
	QMap<QString, QString> datasetGTs;
	for( int i = 1; i < dsDescr.rowCount(); i++ )
		datasetGTs[dsDescr.item( i, gtDatasetColInd )->text()] = dsDescr.item( i, gtGTSegmColumnIndex )->text();
	if( datasetGTs[data->item( 0, datasetColInd )->text()] != "" )
	{
		QString gtSegmFile = m_datasetFolder + "/" + datasetGTs[data->item( 0, datasetColInd )->text()];
		slicer->initializeGT( gtSegmFile );
	}

	//contours
	QVector<double> porosities;
	for( int i = 0; i < data->rowCount(); ++i )
		porosities << data->item( i, m_runsOffset + porosityOffsetInRuns )->text().toDouble();
	iABoxPlotData bpd;
	bpd.CalculateBoxPlot( porosities.data(), porosities.size() );
	int inds[3] = { 0, 0, 0 };
	double deltas[3] = { bpd.max - bpd.min, bpd.max - bpd.min, bpd.max - bpd.min };
	for( int i = 0; i < porosities.size(); ++i )
	{
		double p = porosities[i];
		double curDeltas[3] = { fabs( p - bpd.min ), fabs( p - bpd.med ), fabs( p - bpd.max ) };
		for( int j = 0; j < 3; j++ )
			if( curDeltas[j] < deltas[j] )
			{
				deltas[j] = curDeltas[j];
				inds[j] = i;
			}
	}
	slicer->initBPDChans( masks[inds[0]], masks[inds[1]], masks[inds[2]] );

	//finally
	InitializeGUI();
}

void iASSView::SetData( const QTableWidget * data, QString selText )
{
	m_sliceMgr->removeAll();
	foreach( iASSSlicer * s, m_slicerViews )
	{
		m_slicerViewsLayout->removeWidget( s->container );
		delete s;
	}
	m_slicerViews.clear();

	iASSSlicer * view = new iASSSlicer( selText );
	m_slicerViewsLayout->addWidget( view->container );
	connect( sbRot, SIGNAL( valueChanged( double ) ), view->slicer, SLOT( rotateSlice( double ) ) );
	connect( pushSave, SIGNAL( clicked() ), view->slicer, SLOT( saveAsImage() ) );
	connect( pushMov, SIGNAL( clicked() ), view->slicer, SLOT( saveMovie() ) );
	m_slicerViews.push_back( view );
	
	LoadDataToSlicer( view, data );
	SetDataTo3D();
}

void iASSView::SetCompareData( const QList< QPair<QTableWidget *, QString> > * dataList )
{
	m_sliceMgr->removeAll();
	foreach( iASSSlicer * s, m_slicerViews )
	{
		m_slicerViewsLayout->removeWidget( s->container );
		delete s;
	}
	m_slicerViews.clear();

	for ( int i = 0; i < dataList->size(); ++i )
	{
		iASSSlicer * view = new iASSSlicer( (*dataList)[i].second ) ;
		m_slicerViewsLayout->addWidget( view->container );
		connect( sbRot, SIGNAL( valueChanged( double ) ), view->slicer, SLOT( rotateSlice( double ) ) );
		connect( pushSave, SIGNAL( clicked() ), view->slicer, SLOT( saveAsImage() ) );
		connect( pushMov, SIGNAL( clicked() ), view->slicer, SLOT( saveMovie() ) );
		m_slicerViews.push_back( view );
		LoadDataToSlicer( view, ( *dataList )[i].first );
	}

	SetDataTo3D();
	
	foreach( iASSSlicer *sv, m_slicerViews )
		m_sliceMgr->addToBundle( sv->slicer->GetRenderer() );
}

void iASSView::setSlicerDirection( int cbIndex )
{
	m_modeInd = cbIndex;
	foreach( iASSSlicer * s, m_slicerViews )
		s->changeMode( mode[m_modeInd] );
	InitializeGUI();
}

void iASSView::setSliceSpinBox( int sn )
{
	foreach( iASSSlicer * s, m_slicerViews )
		s->slicer->setSliceNumber( sn );
	QSignalBlocker block( verticalScrollBar );
	verticalScrollBar->setValue( sn );
}

void iASSView::setSliceScrollBar( int sn )
{
	sbNum->repaint();
	verticalScrollBar->repaint();
	foreach( iASSSlicer * s, m_slicerViews )
		s->slicer->setSliceNumber( sn );
	QSignalBlocker block( sbNum );
	sbNum->setValue( sn );
}

void iASSView::setShowMasks( int state )
{
	bool visible = state;
	foreach( iASSSlicer * s, m_slicerViews )
		s->enableMasksChannel( visible );
}

void iASSView::setMasksOpacity( int sliderVal )
{
	double opacity = (double)sliderVal / ( m_SSViewSettings->sMasksOpacity->maximum() - m_SSViewSettings->sMasksOpacity->minimum() );
	foreach( iASSSlicer * s, m_slicerViews )
		s->setMasksOpacity( opacity );
}

void iASSView::setShowGT( int state )
{
	bool visible = state;
	foreach( iASSSlicer * s, m_slicerViews )
		s->enableGTChannel( visible );
}

void iASSView::setGTOpacity( int sliderVal )
{
	double opacity = (double)sliderVal / (m_SSViewSettings->sGTOpacity->maximum() - m_SSViewSettings->sGTOpacity->minimum());
	foreach( iASSSlicer * s, m_slicerViews )
		s->setGTOpacity( opacity );
}

void iASSView::showSettings()
{
	m_SSViewSettings->show();
}

void iASSView::setShowContours( int state )
{
	bool visible = state;
	foreach( iASSSlicer * s, m_slicerViews )
		s->enableContours( visible );
}

void iASSView::attachSegm3DView( iASegm3DView * m_segm3DView )
{
	m_segm3DViewExtrnl = m_segm3DView;
	m_segm3DViewExtrnl->setWindowTitle( "3DView" );
}

void iASSView::SetDataTo3D()
{
	QList<vtkImageData*> imgData;
	QList<vtkPolyData*> polyData;
	QList<vtkPiecewiseFunction*> otf;
	QList<vtkColorTransferFunction*> ctf;
	QStringList slicerNames;
	foreach( iASSSlicer * s, m_slicerViews )
	{
		imgData.push_back( s->masksChan->imgData );
		vtkPolyData * pd = 0;
		if( NeedsDistances() )
			pd = s->GetDeviationPolyData( m_deviationMode );
		else if( NeedsPolyData() )
			pd = s->GetMedPolyData();
		polyData.push_back( pd );
		otf.push_back( s->masksChan->vol_otf );
		ctf.push_back( s->masksChan->tf );
		slicerNames.push_back(s->getSlicerName());
	}
	m_segm3DViewExtrnl->SetDataToVisualize( imgData, polyData, otf, ctf, slicerNames );
}

void iASSView::setShowVolume( int state )
{
	bool visible = state;
	m_segm3DViewExtrnl->ShowVolume( visible );
}

void iASSView::setShowSurface( int state )
{
	bool visible = state;
	UpdatePolyData();
	m_segm3DViewExtrnl->ShowSurface( visible );
}

void iASSView::setShowWireframe( int state )
{
	bool visible = state;
	UpdatePolyData();
	m_segm3DViewExtrnl->ShowWireframe( visible );
}

void iASSView::setDeviationMode( int mode )
{
	m_deviationMode = mode;
	UpdatePolyData();
}

void iASSView::UpdatePolyData()
{
	QList<vtkPolyData*> polyData;
	foreach( iASSSlicer * s, m_slicerViews )
	{
		vtkPolyData * pd = 0;
		if( NeedsDistances() )
			pd = s->GetDeviationPolyData( m_deviationMode );
		else if( NeedsPolyData() )
			pd = s->GetMedPolyData();
		polyData.push_back( pd );
	}
	m_segm3DViewExtrnl->SetPolyData( polyData );
}

void iASSView::setSensitivity( int val )
{
	double sensitivity = (100.0 - val) / 100.0;
	m_segm3DViewExtrnl->SetSensitivity( sensitivity );
}

void iASSView::setRunsOffset( int offset )
{
	m_runsOffset = offset;
}

bool iASSView::NeedsPolyData()
{
	return m_SSViewSettings->cbShowWireframe->isChecked();
}

bool iASSView::NeedsDistances()
{
	return  m_SSViewSettings->cbShowSurface->isChecked();
}
