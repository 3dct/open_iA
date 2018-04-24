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
#include "iABlobCluster.h"

#include <itkImageToVTKImageFilter.h>
#include <itkVTKImageToImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>

#include <vtkPolyDataSilhouette.h>

#define DEFAULT_LABEL_SCALE 15
#define DEFAULT_BLOB_OPACITY 0.3
#define DEFAULT_SILHOUETTE_OPACITY 0.8

iABlobCluster::iABlobCluster( void )
	:
	m_objectType( "Fibers" ),
	m_fiberCount( 0.0 ),
	m_fiberPercentage( 0.0 ),
	m_silhouetteIsOn( true ),
	m_blobIsOn( true ),
	m_labelIsOn( true ),
	m_isSmoothingOn( true ),
	m_renderIndividually( false ),
	m_blurVariance( 1 ),
	m_labelScale( DEFAULT_LABEL_SCALE ),
	m_blobOpacity( DEFAULT_BLOB_OPACITY ),
	m_silhouetteOpacity( DEFAULT_SILHOUETTE_OPACITY ),
	m_blobRenderer( 0 ),
	m_labelRenderer( 0 ),
	m_blobManager( 0 )
{
	// setup variables
	m_countContours = 1;

	m_range[0] = 0.025;
	//m_range[1] = 0.5;

	// initialize members
	m_implicitFunction = iABlobImplicitFunction::New();
	m_sampleFunction = vtkSmartPointer<vtkSampleFunction>::New();
	m_contourFilter = vtkSmartPointer<vtkContourFilter>::New();
	m_contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_contourActor = vtkSmartPointer<vtkActor>::New();
	m_silhouette = vtkSmartPointer<vtkPolyDataSilhouette>::New();
	m_silhouetteMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_silhouetteActor = vtkSmartPointer<vtkActor>::New();
	m_smoother = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	m_polyDataNormals = vtkSmartPointer<vtkPolyDataNormals>::New();

	SetDefaultProperties();
}

iABlobCluster::~iABlobCluster( void )
{
	ResetRenderers();
}

void iABlobCluster::AttachRenderers( vtkSmartPointer<vtkRenderer> blobRen,
									 vtkSmartPointer<vtkRenderer> labelRen )
{
	ResetRenderers();

	m_blobRenderer = blobRen;
	m_labelRenderer = labelRen;

	UpdateRenderer();
}

void iABlobCluster::ResetRenderers( void )
{
	if ( m_blobRenderer )
	{
		//		m_blobRenderer->RemoveAllViewProps();
		m_blobRenderer->RemoveActor( m_contourActor );
		m_blobRenderer->RemoveActor( m_silhouetteActor );
	}
	if ( m_labelRenderer )
	{
		m_label.DetachActorsToRenderers( m_blobRenderer, m_labelRenderer );
	}
}

void iABlobCluster::UpdateRenderer( void )
{
	ResetRenderers();

	if ( !m_blobRenderer )
	{
		return;
	}
	if ( m_renderIndividually )
	{
		if ( m_blobIsOn )
			m_blobRenderer->AddActor( m_contourActor );
		if ( m_silhouetteIsOn )
			m_blobRenderer->AddActor( m_silhouetteActor );
	}
	if ( m_labelIsOn )
		m_label.AttachActorsToRenderers( m_blobRenderer, m_labelRenderer, m_blobRenderer->GetActiveCamera() );
}

void iABlobCluster::SilhouetteOn( void )
{
	m_silhouetteIsOn = true;
	UpdateRenderer();
}

void iABlobCluster::SilhouetteOff( void )
{
	m_silhouetteIsOn = false;
	UpdateRenderer();
}

void iABlobCluster::UpdatePipeline( void )
{
	// create the 0 isosurface
	//m_contourFilter->SetInputConnection (m_sampleFunction->GetOutputPort());
	m_contourFilter->SetInputData( m_imageData );
	m_contourFilter->GenerateValues( m_countContours, m_range );

	if ( m_isSmoothingOn )
	{
		m_smoother->SetInputConnection( m_contourFilter->GetOutputPort() );
		m_polyDataNormals->SetInputConnection( m_smoother->GetOutputPort() );
	}
	else
		m_polyDataNormals->SetInputConnection( m_contourFilter->GetOutputPort() );


	if ( m_renderIndividually )
	{
		// map the contours to graphical primitives
		m_contourMapper->SetInputConnection( m_polyDataNormals->GetOutputPort() );
		m_contourMapper->ScalarVisibilityOff();

		// actor for the contours
		m_contourActor->SetMapper( m_contourMapper );

		// compute the silhouette
		m_silhouette->SetInputConnection( m_polyDataNormals->GetOutputPort() );
		m_silhouette->SetCamera( m_blobRenderer->GetActiveCamera() );

		// map the silhouette
		m_silhouetteMapper->SetInputConnection( m_silhouette->GetOutputPort() );

		// actor for the silhouette
		m_silhouetteActor->SetMapper( m_silhouetteMapper );

	}
}

void iABlobCluster::SetCluster( QVector<FiberInfo> fibres ) const
{
	m_implicitFunction->Reset();

	for ( int i = 0; i < fibres.size(); i++ )
	{
		m_implicitFunction->AddFiberInfo( fibres[i].x1,
										  fibres[i].y1,
										  fibres[i].z1,
										  fibres[i].x2,
										  fibres[i].y2,
										  fibres[i].z2,
										  fibres[i].diameter );
	}
}

vtkProperty* iABlobCluster::GetSurfaceProperty( void )
{
	return m_contourActor->GetProperty();
}

vtkProperty* iABlobCluster::GetSilhouetteProperty( void )
{
	return m_silhouetteActor->GetProperty();
}

void iABlobCluster::SetDefaultProperties( void )
{
	m_contourActor->GetProperty()->SetColor( 1, 0, 0 );
	m_contourActor->GetProperty()->SetOpacity( m_blobOpacity );

	m_silhouetteActor->GetProperty()->SetColor( 0, 0, 0 );
	m_silhouetteActor->GetProperty()->SetLineWidth( 3.0 );
	m_silhouetteActor->GetProperty()->SetOpacity( m_silhouetteOpacity );

	m_smoother->SetNumberOfIterations( 10 );
	m_smoother->BoundarySmoothingOff();
	m_smoother->FeatureEdgeSmoothingOff();
	m_smoother->SetFeatureAngle( 120 );
	m_smoother->SetPassBand( 0.002 );
	m_smoother->NonManifoldSmoothingOn();
	m_smoother->NormalizeCoordinatesOn();

	m_polyDataNormals->ComputePointNormalsOn();
	m_polyDataNormals->ComputeCellNormalsOn();
}

void iABlobCluster::AddClippingPlane( vtkPlane* plane )
{
	//m_implicitFunction->addClippingPlane (plane);
	m_contourMapper->AddClippingPlane( plane );
	m_silhouetteMapper->AddClippingPlane( plane );
}

void iABlobCluster::DrawLabel( void )
{
	double centerPnt[3];
	m_implicitFunction->GetCenter( centerPnt );

	double labeledPnt[3] = { centerPnt[0] + 2.0, centerPnt[1], centerPnt[2] };

	m_label.SetDisplacement( 0.1 );

	m_label.SetLabeledPoint( labeledPnt, centerPnt );

	m_label.SetScale( m_labelScale );

	m_label.qImage = QImage( 400, 100, QImage::Format_ARGB32 );
	m_label.qImage.fill( QColor( 255, 255, 255, 100 ) );

	QPainter painter( &m_label.qImage );
	painter.setRenderHint( QPainter::Antialiasing );
	//painter.fillRect (m_label.qImage.rect(), QColor (255, 255, 255, 100));

	QPen pen( Qt::black );
	pen.setWidthF( 500 );
	pen.setCapStyle( Qt::RoundCap );

	painter.setPen( pen );

	QFont font = QApplication::font();
	font.setPixelSize( 36 );
	font.setWeight( QFont::DemiBold );
	painter.setFont( font );

	QRectF rectTop = m_label.qImage.rect();
	rectTop.setHeight( rectTop.height()*0.5 );
	QRectF rectBottom = QRectF( rectTop.left(), rectTop.bottom(), rectTop.width(), rectTop.height() );

	painter.drawText( rectTop, Qt::AlignCenter, m_name );
	QString statsStr = QString::number( m_fiberCount ) + " " + m_objectType + " (" + QString::number( m_fiberPercentage, 'f', 2 ) + "%)";
	painter.drawText( rectBottom, Qt::AlignCenter, statsStr );

	painter.end();

	m_label.Update();
}

void iABlobCluster::Update( void )
{
	if ( m_labelIsOn )
		DrawLabel();

	UpdateRenderer();

	UpdatePipeline();
}

void iABlobCluster::SetName( QString name )
{
	m_name = name;
}

void iABlobCluster::SetLabel( bool isOn )
{
	m_labelIsOn = isOn;
}

bool iABlobCluster::GetLabel() const
{
	return m_labelIsOn;
}

void iABlobCluster::LabelOn( void )
{
	m_labelIsOn = true;
	DrawLabel();
}

void iABlobCluster::LabelOff( void )
{
	m_labelIsOn = false;
	RemoveLabel();
}

void iABlobCluster::RemoveLabel( void )
{
	// NOT IMPLEMENTED
}

void iABlobCluster::GetDimension( int dimens[3] ) const
{
	for ( int i = 0; i < 3; i++ )
	{
		dimens[i] = m_dimens[i];
	}
}

int* iABlobCluster::GetDimensions()
{
	return m_dimens;
}

void iABlobCluster::GetBounds( double bounds[6] ) const
{
	for ( int i = 0; i < 6; i++ )
	{
		bounds[i] = m_bounds[i];
	}
}

void iABlobCluster::SetDimension( int dimens[3] )
{
	for ( int i = 0; i < 3; i++ )
	{
		m_dimens[i] = dimens[i] > 0 ? dimens[i] : 1;
	}
}

void iABlobCluster::SetBounds( double bounds[6] )
{
	for ( int i = 0; i < 6; i++ )
	{
		m_bounds[i] = bounds[i];
	}
}

void iABlobCluster::SetBlobManager( iABlobManager* blobManager )
{
	m_blobManager = blobManager;
	m_implicitFunction->SetBlobManager( m_blobManager );
}

iABlobImplicitFunction* iABlobCluster::GetImplicitFunction( void )
{
	return m_implicitFunction;
}

void iABlobCluster::CalculateImageData( void )
{
	// sample the function
	m_sampleFunction->SetSampleDimensions( m_dimens );
	m_sampleFunction->SetImplicitFunction( m_implicitFunction );
	m_sampleFunction->SetModelBounds( m_bounds );

	m_imageData = m_sampleFunction->GetOutput();
}

vtkImageData* iABlobCluster::GetImageData( void ) const
{
	return m_imageData;
}

double iABlobCluster::GetRange( void ) const
{
	return m_range[0];
}

void iABlobCluster::SetRange( double range )
{
	m_range[0] = range;
}

void iABlobCluster::ModifiedSampleFunction( void )
{
	m_sampleFunction->Modified();
	m_sampleFunction->Update();
}

void iABlobCluster::SetSmoothing( bool isOn )
{
	m_isSmoothingOn = isOn;
}

bool iABlobCluster::GetSmoothing() const
{
	return m_isSmoothingOn;
}

void iABlobCluster::SetSilhouette( bool isOn )
{
	m_silhouetteIsOn = isOn;
}

bool iABlobCluster::GetSilhouette() const
{
	return m_silhouetteIsOn;
}

void iABlobCluster::SetLabelScale( double labelScale )
{
	m_labelScale = labelScale;
}

double iABlobCluster::GetLabelScale() const
{
	return m_labelScale;
}

void iABlobCluster::SetShowBlob( bool showBlob )
{
	m_blobIsOn = showBlob;
}

bool iABlobCluster::GetShowBlob() const
{
	return m_blobIsOn;
}

void iABlobCluster::SetBlobOpacity( double blobOpacity )
{
	m_blobOpacity = blobOpacity;
	m_contourActor->GetProperty()->SetOpacity( m_blobOpacity );
}

double iABlobCluster::GetBlobOpacity() const
{
	return m_blobOpacity;
}

void iABlobCluster::SetSilhouetteOpacity( double silhouetteOpacity )
{
	m_silhouetteOpacity = silhouetteOpacity;
	m_silhouetteActor->GetProperty()->SetOpacity( m_silhouetteOpacity );
}

void iABlobCluster::SetObjectType( QString type )
{
	m_objectType = type;
}

double iABlobCluster::GetSilhouetteOpacity() const
{
	return m_silhouetteOpacity;
}

void iABlobCluster::SetFiberStats( const double fiberCount, const double fiberPercentage )
{
	m_fiberCount = fiberCount;
	m_fiberPercentage = fiberPercentage;
}

vtkPolyData * iABlobCluster::GetBlobPolyData() const
{
	assert( !m_renderIndividually );
	m_polyDataNormals->Modified();
	m_polyDataNormals->Update();
	vtkPolyData * res = m_polyDataNormals->GetOutput();
	return res;
}


void iABlobCluster::SetRenderIndividually( bool enabled )
{
	if ( m_renderIndividually != enabled )
	{
		m_renderIndividually = enabled;
		if ( enabled )
		{
			UpdateRenderer();
		}
		else
		{
			ResetRenderers();
		}
	}
}

void iABlobCluster::GaussianBlur()
{
	// http://www.vtk.org/Wiki/ITK/Examples/Broken/Images/VTKImageToImageFilter
	// http://www.vtk.org/Wiki/ITK/Examples/IO/ImageToVTKImageFilter
	// http://www.itk.org/Wiki/ITK/Examples/Functions/GaussianBlurImageFunction
	// http://www.itk.org/Wiki/ITK/Examples/Smoothing/DiscreteGaussianImageFilter


	typedef itk::Image<double, 3> ImageType;
	typedef itk::ImageRegionConstIterator< ImageType > ConstIteratorType;
	typedef itk::ImageRegionIterator< ImageType > IteratorType;
	typedef itk::VTKImageToImageFilter<ImageType> VTKImageToImageType;

	VTKImageToImageType::Pointer vtkImageToImageFilter =
		VTKImageToImageType::New();
	vtkImageToImageFilter->SetInput( m_imageData );
	vtkImageToImageFilter->Update();

	typedef itk::DiscreteGaussianImageFilter <
		ImageType, ImageType >  filterType;

	// Create and setup a Gaussian filter
	filterType::Pointer gaussianFilter = filterType::New();
	gaussianFilter->SetInput( vtkImageToImageFilter->GetOutput() );
	gaussianFilter->SetVariance( m_blurVariance );

	typedef itk::ImageToVTKImageFilter<ImageType>       ConnectorType;
	ConnectorType::Pointer connector = ConnectorType::New();
	connector->SetInput( gaussianFilter->GetOutput() );
	connector->Update();
	m_imageData->DeepCopy( connector->GetOutput() );
}

void iABlobCluster::SetGaussianBlurVariance( double blurVariance )
{
	m_blurVariance = blurVariance;
}

double iABlobCluster::GetGaussianBlurVariance() const
{
	return m_blurVariance;
}