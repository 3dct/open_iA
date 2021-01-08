/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASSView.h"

#include "iASegm3DView.h"
#include "iASSSlicer.h"
#include "iASSViewSetings.h"
#include "FeatureAnalyzerHelpers.h"

#include <defines.h>
#include <iABoxPlotData.h>
#include <iAChanData.h>
#include <iAChannelData.h>
#include <iALog.h>
#include <iACSVToQTableWidgetConverter.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <iARendererManager.h>
#include <iAFileUtils.h>

#include <vtkTransform.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageData.h>
#include <vtkMarchingContourFilter.h>
#include <vtkDistancePolyDataFilter.h>

#include <QSettings>

const iASlicerMode mode[3] = { XY, YZ, XZ };
const int extentIndices[3][2] = { { 4, 5 }, { 0, 1 }, { 2, 3 } };

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
	m_sliceMgr( new iARendererManager ),
	m_imgData( vtkSmartPointer<vtkImageData>::New() ),
	m_slicerViewsLayout( new QHBoxLayout(slicerWidget) ),
	m_segm3DViewExtrnl( 0 ),
	m_runsOffset( -10000 )
{
	m_slicerViewsLayout->setMargin( 0 );
	QSettings settings( organisationName, applicationName );
	m_datasetFolder = settings.value( "FeatureAnalyzer/GUI/datasetsFolder", "" ).toString();
	m_resultsFolder = settings.value( "FeatureAnalyzer/GUI/resultsFolder", "" ).toString();
	m_SSViewSettings->cbShowMasks->setChecked( settings.value( "FeatureAnalyzer/GUI/ShowMasks", false ).toBool() );
	m_SSViewSettings->sMasksOpacity->setValue( settings.value( "FeatureAnalyzer/GUI/MasksOpacity", 0 ).toInt() );
	m_SSViewSettings->cbShowGT->setChecked( settings.value( "FeatureAnalyzer/GUI/ShowGT", false ).toBool() );
	m_SSViewSettings->sGTOpacity->setValue( settings.value( "FeatureAnalyzer/GUI/GTOpacity", 0 ).toInt() );
	m_SSViewSettings->cbShowContours->setChecked( settings.value( "FeatureAnalyzer/GUI/ShowContours", false ).toBool() );

	m_SSViewSettings->cbShowVolume->setChecked( settings.value( "FeatureAnalyzer/GUI/ShowVolume", false ).toBool() );
	m_SSViewSettings->cbShowSurface->setChecked( settings.value( "FeatureAnalyzer/GUI/ShowSurface", false ).toBool() );
	m_SSViewSettings->cbShowWireframe->setChecked( settings.value( "FeatureAnalyzer/GUI/ShowWireframe", false ).toBool() );
	m_SSViewSettings->cbDeviation->setCurrentIndex( settings.value( "FeatureAnalyzer/GUI/Deviation", 0 ).toInt() );
	m_deviationMode = m_SSViewSettings->cbDeviation->currentIndex();

	connect(sbNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &iASSView::setSliceSpinBox);
	connect(verticalScrollBar, &QScrollBar::valueChanged, this, &iASSView::setSliceScrollBar);
	connect(cbDir, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iASSView::setSlicerDirection);
	connect(m_SSViewSettings->cbShowMasks, &QCheckBox::stateChanged, this, &iASSView::setShowMasks);
	connect(m_SSViewSettings->sMasksOpacity, &QSlider::valueChanged, this, &iASSView::setMasksOpacity);
	connect(m_SSViewSettings->cbShowGT, &QCheckBox::stateChanged, this, &iASSView::setShowGT);
	connect(m_SSViewSettings->sGTOpacity, &QSlider::valueChanged, this, &iASSView::setGTOpacity);
	connect(m_SSViewSettings->cbShowContours, &QCheckBox::stateChanged, this, &iASSView::setShowContours);
	connect(m_SSViewSettings->cbShowVolume, &QCheckBox::stateChanged, this, &iASSView::setShowVolume);
	connect(m_SSViewSettings->cbShowSurface, &QCheckBox::stateChanged, this, &iASSView::setShowSurface);
	connect(m_SSViewSettings->cbShowWireframe, &QCheckBox::stateChanged, this, &iASSView::setShowWireframe);
	connect(m_SSViewSettings->cbDeviation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iASSView::setDeviationMode);
	connect(m_SSViewSettings->sSensitivity, &QSlider::valueChanged, this, &iASSView::setSensitivity);

	connect(m_SSViewSettings->cbShowMasks, &QCheckBox::stateChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->sMasksOpacity, &QSlider::valueChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->cbShowGT, &QCheckBox::stateChanged, this,   &iASSView::updateSettings);
	connect(m_SSViewSettings->sGTOpacity, &QSlider::valueChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->cbShowContours, &QCheckBox::stateChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->cbShowVolume, &QCheckBox::stateChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->cbShowSurface, &QCheckBox::stateChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->cbShowWireframe, &QCheckBox::stateChanged, this, &iASSView::updateSettings);
	connect(m_SSViewSettings->cbDeviation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iASSView::updateSettings);
	connect(m_SSViewSettings->sSensitivity, &QSlider::valueChanged, this, &iASSView::updateSettings);

	connect(tbSettings, &QToolButton::clicked, this, &iASSView::showSettings);
}

iASSView::~iASSView()
{
	for (iASSSlicer* s : m_slicerViews)
	{
		delete s;
	}
	updateSettings();
}

void iASSView::updateSettings()
{
	QSettings settings( organisationName, applicationName );
	settings.setValue( "FeatureAnalyzer/GUI/ShowMasks", m_SSViewSettings->cbShowMasks->isChecked() );
	settings.setValue( "FeatureAnalyzer/GUI/MasksOpacity", m_SSViewSettings->sMasksOpacity->value() );
	settings.setValue( "FeatureAnalyzer/GUI/ShowGT", m_SSViewSettings->cbShowGT->isChecked() );
	settings.setValue( "FeatureAnalyzer/GUI/GTOpacity", m_SSViewSettings->sGTOpacity->value() );
	settings.setValue( "FeatureAnalyzer/GUI/ShowContours", m_SSViewSettings->cbShowContours->isChecked() );

	settings.setValue( "FeatureAnalyzer/GUI/ShowVolume", m_SSViewSettings->cbShowVolume->isChecked() );
	settings.setValue( "FeatureAnalyzer/GUI/ShowSurface", m_SSViewSettings->cbShowSurface->isChecked() );
	settings.setValue( "FeatureAnalyzer/GUI/ShowWireframe", m_SSViewSettings->cbShowWireframe->isChecked() );
	settings.setValue( "FeatureAnalyzer/GUI/Deviation", m_SSViewSettings->cbDeviation->currentIndex() );
}

void iASSView::BuildDefaultTF( vtkSmartPointer<vtkImageData> & imgData, vtkSmartPointer<vtkColorTransferFunction> & tf, QColor color )
{
	if (!imgData)
	{
		LOG(lvlError, "Image data is nullptr!");
		return;
	}
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

void iASSView::LoadDataToSlicer( iASSSlicer * slicer, const QTableWidget * dataTable )
{
	//data itself
	m_datasetFile = m_datasetFolder + "/" + dataTable->item( 0, datasetColInd )->text();
	loadImageData( m_datasetFile, m_imgData );
	BuildDefaultTF( m_imgData, m_slicerTF );
	slicer->initialize( m_imgData, m_slicerTF );

	//masks channel
	QStringList masks;
	for (int i = 0; i < dataTable->rowCount(); ++i)
	{
		masks << dataTable->item(i, m_runsOffset + maskOffsetInRuns)->text();
	}

	slicer->initializeMasks( masks );

	//gt channel
	QTableWidget dsDescr;
	iACSVToQTableWidgetConverter::loadCSVFile( m_resultsFolder + "/DatasetDescription.csv", &dsDescr );
	QMap<QString, QString> datasetGTs;
	for (int i = 1; i < dsDescr.rowCount(); i++)
	{
		datasetGTs[dsDescr.item(i, gtDatasetColInd)->text()] = dsDescr.item(i, gtGTSegmColumnIndex)->text();
	}
	if( datasetGTs[dataTable->item( 0, datasetColInd )->text()] != "" )
	{
		QString gtSegmFile = m_datasetFolder + "/" + datasetGTs[dataTable->item( 0, datasetColInd )->text()];
		slicer->initializeGT( gtSegmFile );
	}

	//contours
	QVector<double> porosities;
	for (int i = 0; i < dataTable->rowCount(); ++i)
	{
		porosities << dataTable->item(i, m_runsOffset + porosityOffsetInRuns)->text().toDouble();
	}
	iABoxPlotData bpd;
	bpd.CalculateBoxPlot( porosities.data(), porosities.size() );
	int inds[3] = { 0, 0, 0 };
	double deltas[3] = { bpd.max - bpd.min, bpd.max - bpd.min, bpd.max - bpd.min };
	for( int i = 0; i < porosities.size(); ++i )
	{
		double p = porosities[i];
		double curDeltas[3] = { fabs( p - bpd.min ), fabs( p - bpd.med ), fabs( p - bpd.max ) };
		for (int j = 0; j < 3; j++)
		{
			if (curDeltas[j] < deltas[j])
			{
				deltas[j] = curDeltas[j];
				inds[j] = i;
			}
		}
	}
	slicer->initBPDChans( masks[inds[0]], masks[inds[1]], masks[inds[2]] );

	//finally
	InitializeGUI();
}

void iASSView::SetData( const QTableWidget * dataTable, QString selText )
{
	m_sliceMgr->removeAll();
	for (iASSSlicer* s : m_slicerViews)
	{
		m_slicerViewsLayout->removeWidget( s->container );
		delete s;
	}
	m_slicerViews.clear();

	iASSSlicer * view = new iASSSlicer( selText, m_slicerTransform);
	m_slicerViewsLayout->addWidget( view->container );
	connect(sbRot, QOverload<double>::of(&QDoubleSpinBox::valueChanged), view->slicer, &iASlicer::rotateSlice);
	connect(pushSave, &QToolButton::clicked, view->slicer, &iASlicer::saveAsImage);
	connect(pushMov, &QToolButton::clicked, view->slicer, &iASlicer::saveMovie);
	m_slicerViews.push_back( view );

	LoadDataToSlicer(view, dataTable);
	SetDataTo3D();
}

void iASSView::SetCompareData( const QList< QPair<QTableWidget *, QString> > * dataList )
{
	m_sliceMgr->removeAll();
	for (iASSSlicer* s : m_slicerViews)
	{
		m_slicerViewsLayout->removeWidget( s->container );
		delete s;
	}
	m_slicerViews.clear();

	for ( int i = 0; i < dataList->size(); ++i )
	{
		iASSSlicer * view = new iASSSlicer( (*dataList)[i].second, m_slicerTransform ) ;
		m_slicerViewsLayout->addWidget( view->container );
		connect(sbRot, QOverload<double>::of(&QDoubleSpinBox::valueChanged), view->slicer, &iASlicer::rotateSlice);
		connect(pushSave, &QToolButton::clicked, view->slicer, &iASlicer::saveAsImage);
		connect(pushMov,  &QToolButton::clicked, view->slicer, &iASlicer::saveMovie);
		m_slicerViews.push_back( view );
		LoadDataToSlicer( view, ( *dataList )[i].first );
	}

	SetDataTo3D();

	for (iASSSlicer* s : m_slicerViews)
	{
		m_sliceMgr->addToBundle( s->slicer->renderer() );
	}
}

void iASSView::setSlicerDirection( int cbIndex )
{
	m_modeInd = cbIndex;
	for (iASSSlicer* s : m_slicerViews)
	{
		s->changeMode( mode[m_modeInd] );
	}
	InitializeGUI();
}

void iASSView::setSliceSpinBox( int sn )
{
	for (iASSSlicer* s : m_slicerViews)
	{
		s->slicer->setSliceNumber( sn );
	}
	QSignalBlocker block( verticalScrollBar );
	verticalScrollBar->setValue( sn );
}

void iASSView::setSliceScrollBar( int sn )
{
	sbNum->repaint();
	verticalScrollBar->repaint();
	for (iASSSlicer* s : m_slicerViews)
	{
		s->slicer->setSliceNumber( sn );
	}
	QSignalBlocker block( sbNum );
	sbNum->setValue( sn );
}

void iASSView::setShowMasks( int state )
{
	bool visible = state;
	for (iASSSlicer* s : m_slicerViews)
	{
		s->enableMasksChannel( visible );
	}
}

void iASSView::setMasksOpacity( int sliderVal )
{
	double opacity = (double)sliderVal / ( m_SSViewSettings->sMasksOpacity->maximum() - m_SSViewSettings->sMasksOpacity->minimum() );
	for (iASSSlicer* s : m_slicerViews)
	{
		s->setMasksOpacity( opacity );
	}
}

void iASSView::setShowGT( int state )
{
	bool visible = state;
	for (iASSSlicer* s : m_slicerViews)
	{
		s->enableGTChannel( visible );
	}
}

void iASSView::setGTOpacity( int sliderVal )
{
	double opacity = (double)sliderVal / (m_SSViewSettings->sGTOpacity->maximum() - m_SSViewSettings->sGTOpacity->minimum());
	for (iASSSlicer* s : m_slicerViews)
	{
		s->setGTOpacity( opacity );
	}
}

void iASSView::showSettings()
{
	m_SSViewSettings->show();
}

void iASSView::setShowContours( int state )
{
	bool visible = state;
	for (iASSSlicer* s : m_slicerViews)
	{
		s->enableContours( visible );
	}
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
	for (iASSSlicer* s : m_slicerViews)
	{
		imgData.push_back( s->masksChan->imgData );
		vtkPolyData * pd = 0;
		if (NeedsDistances())
		{
			pd = s->GetDeviationPolyData(m_deviationMode);
		}
		else if (NeedsPolyData())
		{
			pd = s->GetMedPolyData();
		}
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

void iASSView::setDeviationMode( int deviationMode )
{
	m_deviationMode = deviationMode;
	UpdatePolyData();
}

void iASSView::UpdatePolyData()
{
	QList<vtkPolyData*> polyData;
	for (iASSSlicer* s : m_slicerViews)
	{
		vtkPolyData * pd = 0;
		if (NeedsDistances())
		{
			pd = s->GetDeviationPolyData(m_deviationMode);
		}
		else if (NeedsPolyData())
		{
			pd = s->GetMedPolyData();
		}
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
