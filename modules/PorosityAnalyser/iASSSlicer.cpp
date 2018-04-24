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
#include "iASSSlicer.h"

#include "iAChanData.h"
#include "iAChannelVisualizationData.h"
#include "iAConnector.h"
#include "iASlicer.h"
#include "iASlicerSettings.h"
#include "PorosityAnalyserHelpers.h"
#include "io/iAITKIO.h"

#include <itkAddImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkDivideImageFilter.h>
#include <itkImage.h>
#include <itkImageToVTKImageFilter.h>

#include <vtkColorTransferFunction.h>
#include <vtkDistancePolyDataFilter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkMarchingContourFilter.h>
#include <vtkMetaImageReader.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkTransform.h>

#include <QVBoxLayout>
#include <QLabel>

extern const iAChannelID MasksChanID = ch_Concentration0;
extern const iAChannelID GTChanID = ch_Concentration1;

extern const iAChannelID minChanID = ch_Concentration2;
extern const iAChannelID medChanID = ch_Concentration3;
extern const iAChannelID maxChanID = ch_Concentration4;
const double contourValue = 0.99999;

const QList<QColor> brewer_YlOrRd = QList<QColor>() \
<< QColor( "#ffffcc" ) \
<< QColor( "#ffeda0" ) \
<< QColor( "#fed976" ) \
<< QColor( "#feb24c" ) \
<< QColor( "#fd8d3c" ) \
<< QColor( "#fc4e2a" ) \
<< QColor( "#e31a1c" ) \
<< QColor( "#bd0026" ) \
<< QColor( "#800026" );

const QList<QColor> brewer_YlGnBu = QList<QColor>() \
<< QColor( "#ffffd9" ) \
<< QColor( "#edf8b1" ) \
<< QColor( "#c7e9b4" ) \
<< QColor( "#7fcdbb" ) \
<< QColor( "#41b6c4" ) \
<< QColor( "#1d91c0" ) \
<< QColor( "#225ea8" ) \
<< QColor( "#253494" ) \
<< QColor( "#081d58" );

const QList<QColor> brewer_RdPu = QList<QColor>() \
<< QColor( "#fff7f3" ) \
<< QColor( "#fde0dd" ) \
<< QColor( "#fcc5c0" ) \
<< QColor( "#fa9fb5" ) \
<< QColor( "#f768a1" ) \
<< QColor( "#dd3497" ) \
<< QColor( "#ae017e" ) \
<< QColor( "#7a0177" ) \
<< QColor( "#49006a" );

extern void loadImageData( const char * fileName, vtkSmartPointer<vtkImageData> & imgData )
{
	vtkSmartPointer<vtkMetaImageReader> reader = vtkSmartPointer<vtkMetaImageReader>::New();
	reader->SetFileName( fileName );
	reader->Update();
	imgData = reader->GetOutput();
}

iASSSlicer::iASSSlicer( const QString slicerName ) :
	masksChan( new iAChanData( brewer_RdPu, MasksChanID ) ),
	gtChan( new iAChanData( QColor( 0, 0, 0 ), QColor( 255, 255, 0 ), GTChanID ) ),
	minChan( new iAChanData( QColor( 0, 0, 0 ), QColor( 80, 80, 80 ), minChanID ) ),
	medChan( new iAChanData( QColor( 0, 0, 0 ), QColor( 160, 160, 160 ), medChanID ) ),
	maxChan( new iAChanData( QColor( 0, 0, 0 ), QColor( 240, 240, 240 ), maxChanID ) ),
	medContour( vtkSmartPointer<vtkMarchingContourFilter>::New() ),
	minContour( vtkSmartPointer<vtkMarchingContourFilter>::New() ),
	maxContour( vtkSmartPointer<vtkMarchingContourFilter>::New() ),
	distFilterMax( vtkSmartPointer<vtkDistancePolyDataFilter>::New() ),
	distFilterMin( vtkSmartPointer<vtkDistancePolyDataFilter>::New() ),
	m_SlicerName(slicerName)
{
	container = new QWidget();
	containerLayout = new QVBoxLayout (container);
	container->setLayout( containerLayout );
	
	QLabel * selTextLabel = new QLabel( m_SlicerName, container );
	selTextLabel->setAlignment( Qt::AlignCenter );
	selTextLabel->setFixedHeight( 15 );
	selTextLabel->setStyleSheet( "font-weight: bold;" );

	wgt = new QWidget(container);
	slicer = new iASlicer( wgt, iASlicerMode::XY, wgt );

	medContour->SetNumberOfContours( 1 );
	medContour->SetValue( 0, contourValue );
	medContour->UseScalarTreeOn();
	medContour->SetComputeGradients( false );
	medContour->SetComputeNormals( false );
	
	minContour->SetNumberOfContours( 1 );
	minContour->SetValue( 0, contourValue );
	minContour->UseScalarTreeOn();
	minContour->SetComputeGradients( false );
	minContour->SetComputeNormals( false );
	
	maxContour->SetNumberOfContours( 1 );
	maxContour->SetValue( 0, contourValue );
	maxContour->UseScalarTreeOn();
	maxContour->SetComputeGradients( false );
	maxContour->SetComputeNormals( false );

	containerLayout->addWidget( selTextLabel );
	containerLayout->addWidget( wgt );
}

void iASSSlicer::enableMasksChannel( bool isEnabled )
{
	slicer->enableChannel( masksChan->id, isEnabled );
	update();
}

void iASSSlicer::enableGTChannel( bool isEnabled )
{
	slicer->enableChannel( gtChan->id, isEnabled );
	update();
}

void iASSSlicer::setMasksOpacity( double opacity )
{
	slicer->setChannelOpacity( masksChan->id, opacity );
	update();
}

void iASSSlicer::setGTOpacity( double opacity )
{
	slicer->setChannelOpacity( gtChan->id, opacity );
	update();
}

iASSSlicer::~iASSSlicer()
{
	delete slicer;
}

void iASSSlicer::changeMode( iASlicerMode mode )
{
	slicer->ChangeMode( mode );
	slicer->update();
}

void iASSSlicer::initialize( vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkTransform> transform, vtkSmartPointer<vtkColorTransferFunction> tf )
{
	slicer->setup( iASingleSlicerSettings() );
	slicer->initializeData(img, transform, tf);
	slicer->initializeWidget( img );
	slicer->update();
}

void iASSSlicer::initializeChannel( iAChanData * chData )
{
	chData->InitTFs();
	slicer->initializeChannel( chData->id, chData->visData.data() );
}

void iASSSlicer::initBPDChans( const char * minFile, const char * medFile, const char * maxFile )
{
	loadImageData( minFile, minChan->imgData );
	loadImageData( medFile, medChan->imgData );
	loadImageData( maxFile, maxChan->imgData );

	iAChanData * contourChans[3] = { minChan.data(), medChan.data(), maxChan.data() };

	double lineWidths[3] = { 3, 5, 3 };
	double contRGBs[3][3] = { { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 } };

	for( int i = 0; i < 3; ++i )
	{
		initializeChannel( contourChans[i] );
		slicer->enableChannel( contourChans[i]->id, true );
		slicer->setChannelOpacity( contourChans[i]->id, 0.0 );

		iAChannelSlicerData * chanSData = slicer->GetChannel( contourChans[i]->id );
		chanSData = slicer->GetChannel( contourChans[i]->id );
		chanSData->SetContourLineParams( lineWidths[i] );
		chanSData->SetContoursColor( contRGBs[i] );
		chanSData->SetContoursOpacity( 0.8 );
		chanSData->SetContours( 1, &contourValue );
		chanSData->SetShowContours( true );
	}
	medContour->SetInputData( medChan->imgData );
	minContour->SetInputData( minChan->imgData );
	maxContour->SetInputData( maxChan->imgData );
	distFilterMax->SetInputConnection( 0, medContour->GetOutputPort() );
	distFilterMax->SetInputConnection( 1, maxContour->GetOutputPort() );
	distFilterMin->SetInputConnection( 0, medContour->GetOutputPort() );
	distFilterMin->SetInputConnection( 1, minContour->GetOutputPort() );

	slicer->update();
}

void iASSSlicer::computeAggregatedImageData( const QStringList & filesList )
{
	vtkSmartPointer<vtkImageData> imgData = masksChan->imgData;
	typedef itk::Image< unsigned char, imgDim >   SumImageType;
	//first use itk filters to compute aggregated image
	typedef itk::AddImageFilter<SumImageType, MaskImageType, SumImageType> AddImageFilter;
	typedef itk::DivideImageFilter<MaskImageType, MaskImageType, MaskImageType> DivideImageFilter;

	ScalarPixelType pixelType;
	ImagePointer lastOutput = iAITKIO::readFile( filesList.first(), pixelType, true);
	{
		MaskImageType * castInput = dynamic_cast<MaskImageType*>(lastOutput.GetPointer());
		typedef itk::CastImageFilter< MaskImageType, SumImageType> CastToIntFilterType;
		CastToIntFilterType::Pointer toSumType = CastToIntFilterType::New();
		toSumType->SetInput( castInput );
		toSumType->Update();
		lastOutput = toSumType->GetOutput();
	}
	for( int i = 1; i < filesList.size(); ++i )
	{
		ImagePointer input = iAITKIO::readFile( filesList[i], pixelType, true);
		AddImageFilter::Pointer add = AddImageFilter::New();
		SumImageType * input1 = dynamic_cast<SumImageType*>(lastOutput.GetPointer());
		MaskImageType * input2 = dynamic_cast<MaskImageType*>(input.GetPointer());
		assert( input1 && input2 );
		add->SetInput1( input1 );
		add->SetInput2( input2 );
		add->Update();
		lastOutput = add->GetOutput();
	}

	//convert from itk to vtk
	SumImageType * input = dynamic_cast<SumImageType*>(lastOutput.GetPointer());
	assert( input );
	// 	typedef itk::ImageToVTKImageFilter<MaskImageType> ImageToVTK;
	// 	ImageToVTK::Pointer imageToVTK = ImageToVTK::New();
	// 	imageToVTK->SetInput( input );
	// 	imageToVTK->Update();
	// 	imgData->DeepCopy( imageToVTK->GetOutput() );
	iAConnector con; con.SetImage( input );
	imgData->DeepCopy( con.GetVTKImage() );
}

void iASSSlicer::initializeMasks( QStringList & masks )
{
	computeAggregatedImageData( masks );
	initializeChannel( masksChan.data() );
	vtkScalarBarWidget * sbw = masksChan->scalarBarWgt;
	sbw->SetRepositionable( true );
	sbw->SetResizable( true );
	sbw->GetScalarBarRepresentation()->SetOrientation( 1 );
	sbw->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue( 0.02, 0.2 );
	sbw->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue( 0.06, 0.75 );
	sbw->GetScalarBarActor()->SetTitle( "Mask count" );
	sbw->GetScalarBarActor()->SetNumberOfLabels( 4 );
	sbw->SetInteractor( slicer->GetRenderWindow()->GetInteractor() );
	masksChan->scalarBarWgt->SetEnabled( true );
}

void iASSSlicer::initializeGT( const char * fileName )
{
	loadImageData( fileName, gtChan->imgData );
	initializeChannel( gtChan.data() );
}

void iASSSlicer::enableContours( bool isEnabled )
{
	iAChanData * contourChans[3] = { minChan.data(), maxChan.data(), medChan.data() };
	for( int i = 0; i < 3; ++i )
	{
		iAChannelSlicerData * contourChan = slicer->GetChannel( contourChans[i]->id );
		contourChan->SetShowContours( isEnabled );
		slicer->enableChannel( contourChans[i]->id, isEnabled );
	}
	update();
}

void iASSSlicer::update()
{
	slicer->update();
}

vtkPolyData * iASSSlicer::GetDeviationPolyData( int deviationMode )
{
	vtkPolyData * res = 0;
	switch( deviationMode )
	{
		case 0://maximum
			distFilterMax->Update();
			distFilterMax->GetOutput()->GetPointData()->GetScalars()->SetName( "Colors" );
			res = distFilterMax->GetOutput();
			break;
		case 1://minimum
			distFilterMin->Update();
			distFilterMin->GetOutput()->GetPointData()->GetScalars()->SetName( "Colors" );
			res = distFilterMin->GetOutput();
			break;
	}
	return res;
}

vtkPolyData * iASSSlicer::GetMedPolyData()
{
	medContour->Update();
	return medContour->GetOutput();
}

QString iASSSlicer::getSlicerName()
{
	return  m_SlicerName;
}
