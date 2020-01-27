/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iABlobManager.h"

#include "iABlobCluster.h"

#include <iAMovieHelper.h>
#include <iARenderer.h>
#include <mdichild.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkGaussianBlurImageFunction.h>
#include <itkImageRegionIterator.h>

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCamera.h>
#include <vtkDepthSortPolyData.h>
#include <vtkGenericMovieWriter.h>
#include <vtkLookupTable.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataSilhouette.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

#include <QSettings>
#include <QWidget>

#include <limits>

#define MAX_VALUE_COMPONENT 0
#define DELTA_COMPONENT 1

namespace
{
	const QString DepthPeelingKey("FeatureScout/Blobs/UseDepthPeeling");
}


// Constructor
iABlobManager::iABlobManager( void )
	:m_range( 3000 ),
	m_overlapThreshold( 100 ),
	m_blurVariance( 32.0 ),
	m_boundsProtrusionCoef( 1 ),
	m_isSmoothingEnabled( true ),
	m_isGaussianBlurEnabled( true ),
	m_isSilhoetteEnabled( true ),
	m_isBlobBodyEnabled( true ),
	m_isLabelingEnabled( true ),
	m_blobOpacity( 0.3 ),
	m_silhouetteOpacity( 0.8 ),
	m_depthPeelingEnabled( true ),
	m_blobRen( 0 ),
	m_labelRen( 0 )
{
	m_imageMask = vtkSmartPointer<vtkImageData>::New();
	m_appendedBlobsPD = vtkSmartPointer<vtkAppendPolyData>::New();
	m_blobsLT = vtkSmartPointer<vtkLookupTable>::New();
	m_blobsDepthSort = vtkSmartPointer<vtkDepthSortPolyData>::New();
	m_blobsMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_blobsActor = vtkSmartPointer<vtkActor>::New();
	m_silhouette = vtkSmartPointer<vtkPolyDataSilhouette>::New();
	m_silhouetteMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_silhouetteActor = vtkSmartPointer<vtkActor>::New();

	QSettings settings;
	m_depthPeelingEnabled = settings.value( DepthPeelingKey, true ).toBool();
}

void iABlobManager::Update( void )
{
	for ( int i = 0; i < m_blobsList.count(); i++ )
	{
		m_blobsList[i]->AttachRenderers( m_blobRen, m_labelRen );

		m_blobsList[i]->CalculateImageData();
		m_blobsList[i]->ModifiedSampleFunction();
		m_blobsList[i]->Update();
	}

	if ( m_overlappingEnabled )
		SmartOverlapping();

	if ( m_depthPeelingEnabled )
	{
		for ( int i = 0; i < m_blobsList.count(); i++ )
		{
			m_blobsList[i]->Update();
		}
	}
	else
	{
		vtkPolyData ** blobs_pd;
		const int blobCount = m_blobsList.count();
		blobs_pd = new vtkPolyData*[blobCount];
		m_appendedBlobsPD->RemoveAllInputs();
		m_blobsLT->SetNumberOfTableValues( blobCount );
		m_blobsLT->SetTableRange( 0, blobCount );
		m_blobsLT->Build();
		for ( int i = 0; i < blobCount; i++ )
		{
			m_blobsList[i]->Update();

			//lookup table
			double rgba[4];
			m_blobsList[i]->GetSurfaceProperty()->GetColor( rgba[0], rgba[1], rgba[2] );

			if ( !m_blobsList[i]->GetShowBlob() )
			{
				rgba[3] = 0.0;
			}
			else
			{
				rgba[3] = m_blobsList[i]->GetSurfaceProperty()->GetOpacity();
			}

			m_blobsLT->SetTableValue( i, rgba );

			//scalars
			blobs_pd[i] = m_blobsList[i]->GetBlobPolyData();
			vtkSmartPointer<vtkUnsignedCharArray> colIDs = vtkSmartPointer<vtkUnsignedCharArray>::New();
			colIDs->SetNumberOfComponents( 1 );
			colIDs->SetName( "ColorIDs" );
			unsigned char val;
			for ( int cellInd = 0; cellInd < blobs_pd[i]->GetNumberOfPoints(); ++cellInd )
			{
				val = i;
				colIDs->InsertNextTypedTuple(&val);
			}
			blobs_pd[i]->GetPointData()->SetScalars( colIDs );
			m_appendedBlobsPD->AddInputData( blobs_pd[i] );
		}

		m_appendedBlobsPD->Modified();
		m_appendedBlobsPD->Update();
		m_blobsMapper->UseLookupTableScalarRangeOn();
		m_blobsMapper->SetLookupTable( m_blobsLT );

		m_silhouetteActor->GetProperty()->SetColor( 0.0, 0.0, 0.0 );
		m_silhouetteActor->GetProperty()->SetLineWidth( 3.0 );
		m_silhouetteActor->GetProperty()->SetOpacity( m_silhouetteOpacity );
		m_blobsActor->SetVisibility( true );
		m_silhouetteActor->SetVisibility( m_isSilhoetteEnabled );

		delete[] blobs_pd;
	}
}

void iABlobManager::AddBlob( iABlobCluster* blob )
{
	blob->SetRenderIndividually( m_depthPeelingEnabled );
	m_blobsList.append( blob );
}

void iABlobManager::UpdateBlobSettings( iABlobCluster* blob )
{
	blob->SetBounds( this->GetBoundsProtrusion() );
	blob->SetDimension( this->GetDimensions() );
	blob->SetRange( m_range );
	blob->SetSmoothing( m_isSmoothingEnabled );
	blob->SetSilhouette( m_isSilhoetteEnabled );
	blob->SetLabel( m_isLabelingEnabled );
	blob->SetShowBlob( m_isBlobBodyEnabled );
	blob->SetBlobOpacity( m_blobOpacity );
	blob->SetSilhouetteOpacity( m_silhouetteOpacity );
	blob->SetGaussianBlurVariance( m_blurVariance );
}

void iABlobManager::RemoveBlob( iABlobCluster* blob )
{
	m_blobsList.removeOne( blob );

	// if depth peeling is enabled, the individual blobs do their rendering,
	// and with deleting the blob also its rendering disappears. if we don't
	// use it however,...
	if ( !m_depthPeelingEnabled )
	{
		// ...we need to hide the blobs manually
		if ( m_blobsList.count() == 0 )
		{
			m_blobsActor->SetVisibility( 0 );
			m_silhouetteActor->SetVisibility( 0 );
		}
		else
		{
			Update();
		}
	}
}

QList<iABlobCluster*>* iABlobManager::GetListObBlobClusters( void )
{
	return &m_blobsList;
}

bool iABlobManager::SmartOverlapping( void )
{
	int size = m_blobsList.count();

	if ( size == 0 )
		return false;

	// we propose that all blobs have same data type
	int extent[6];
	m_blobsList[0]->GetImageData()->GetExtent( extent );
	// 	InitializeMask (extent);
	// 	for (int i = 0; i < size; i++)
	// 	{
	// 		AddBlobToMask (m_blobsList[i]->GetImageData());
	// 	}
	//GaussianBlurMask ();


	for ( int i = 0; i < size; i++ )
		m_blobsList[i]->GaussianBlur();

	//smart separation
	const double infinity = std::numeric_limits<double>::max(); //m_range + 10;
	if ( size > 1 )
	{
		for ( int x = extent[0]; x <= extent[1]; x++ )
		{
			for ( int y = extent[2]; y <= extent[3]; y++ )
			{
				for ( int z = extent[4]; z <= extent[5]; z++ )
				{
					double d1, d2, d1_out, d2_out, delta;
					for ( int i = 0; i < size; ++i )
					{
						d1 = d1_out = m_blobsList[i]->GetImageData()->GetScalarComponentAsDouble( x, y, z, MAX_VALUE_COMPONENT );
						for ( int j = 0; j < size && i != j; ++j )
						{
							d2 = d2_out = m_blobsList[j]->GetImageData()->GetScalarComponentAsDouble( x, y, z, MAX_VALUE_COMPONENT );
							delta = fabs( d1 - d2 );
							if ( delta < m_overlapThreshold )
								d1_out = d2_out = infinity;
							else
							{
								d1 > d2 ? d1_out = infinity : d2_out = infinity;
							}
							m_blobsList[j]->GetImageData()->SetScalarComponentFromDouble( x, y, z, MAX_VALUE_COMPONENT, d2_out );
						}
						m_blobsList[i]->GetImageData()->SetScalarComponentFromDouble( x, y, z, MAX_VALUE_COMPONENT, d1_out );
					}
				}
			}
		}
	}

	return true;
}

void iABlobManager::InitializeMask( int extent[6] )
{
	m_imageMask->SetExtent( extent );
	m_imageMask->AllocateScalars( VTK_DOUBLE, 2 );

	for ( int x = extent[0]; x <= extent[1]; x++ )
	{
		for ( int y = extent[2]; y <= extent[3]; y++ )
		{
			for ( int z = extent[4]; z <= extent[5]; z++ )
			{
				m_imageMask->SetScalarComponentFromDouble( x, y, z, MAX_VALUE_COMPONENT, 0 );
			}
		}
	}
}

void iABlobManager::AddBlobToMask( vtkImageData* imageData )
{
	int extent[6];
	//m_imageMask->SetNumberOfScalarComponents (2);
	//m_imageMask->SetScalarTypeToDouble ();
	m_imageMask->GetExtent( extent );
	//m_imageMask->AllocateScalars ();
	int extent2[6];
	imageData->GetExtent( extent2 );

	double maskVal, dataVal, oldValue;
	for ( int x = extent[0]; x <= extent[1]; x++ )
	{
		for ( int y = extent[2]; y <= extent[3]; y++ )
		{
			for ( int z = extent[4]; z <= extent[5]; z++ )
			{
				maskVal = m_imageMask->GetScalarComponentAsDouble(
					x, y, z, MAX_VALUE_COMPONENT );

				dataVal = imageData->GetScalarComponentAsDouble(
					x, y, z, MAX_VALUE_COMPONENT );

				if ( dataVal > maskVal )
				{
					oldValue = m_imageMask->GetScalarComponentAsDouble(
						x, y, z, MAX_VALUE_COMPONENT );

					m_imageMask->SetScalarComponentFromDouble(
						x, y, z, MAX_VALUE_COMPONENT, dataVal );

					m_imageMask->SetScalarComponentFromDouble(
						x, y, z, DELTA_COMPONENT, dataVal - oldValue );
				}
			}
		}
	}
}

void iABlobManager::OverlapWithMask( vtkImageData* imageData )
{
	int extent[6];
	m_imageMask->GetExtent( extent );

	double maskVal, dataVal, deltaValue;
	for ( int x = extent[0]; x <= extent[1]; x++ )
	{
		for ( int y = extent[2]; y <= extent[3]; y++ )
		{
			for ( int z = extent[4]; z <= extent[5]; z++ )
			{
				maskVal = m_imageMask->GetScalarComponentAsDouble( x, y, z, MAX_VALUE_COMPONENT );
				deltaValue = m_imageMask->GetScalarComponentAsDouble( x, y, z, DELTA_COMPONENT );
				dataVal = imageData->GetScalarComponentAsDouble( x, y, z, MAX_VALUE_COMPONENT );

				if ( dataVal < maskVal || deltaValue < m_overlapThreshold )
				{
					//double val = qMax(m_overlapThreshold - abs (dataVal - m_overlapThreshold), 0.0);

					imageData->SetScalarComponentFromDouble( x, y, z, MAX_VALUE_COMPONENT, 0 );
				}

				// 				float m_overlapThreshold2 = m_overlapThreshold * 2;
				//
				// 				if (dataVal < maskVal)
				// 				{
				// 					imageData->SetScalarComponentFromDouble (x, y, z, MAX_VALUE_COMPONENT, 0);
				// 				}
				// 				else if (deltaValue < m_overlapThreshold)
				// 				{
				// 					imageData->SetScalarComponentFromDouble (x, y, z, MAX_VALUE_COMPONENT, 0);
				// 				}
				// 				else if (deltaValue < m_overlapThreshold2)
				// 				{
				// 					float a1 = m_overlapThreshold2;
				// 					float a2 = m_overlapThreshold;
				//
				// 					imageData->SetScalarComponentFromDouble (x, y, z, MAX_VALUE_COMPONENT, dataVal * (deltaValue - a1) / (a2 - a1));
				// 				}
			}
		}
	}
}

void iABlobManager::SetOverlapThreshold( double overlapThreshold )
{
	m_overlapThreshold = overlapThreshold;
}

double iABlobManager::GetOverlapThreshold( void )
{
	return m_overlapThreshold;
}

void iABlobManager::SetGaussianBlurVariance( double variance )
{
	m_blurVariance = variance;
}

double iABlobManager::GetGaussianBlurVariance( void )
{
	return m_blurVariance;
}

void iABlobManager::SetOverlappingEnabled( bool isEnable )
{
	m_overlappingEnabled = isEnable;
}

bool iABlobManager::OverlappingIsEnabled( void )
{
	return m_overlappingEnabled;
}

void iABlobManager::SetRange( double range )
{
	m_range = range;
}

double iABlobManager::GetRange( void )
{
	return m_range;
}

void iABlobManager::SetBounds( double const * bounds )
{
	for ( int i = 0; i < 6; i++ )
	{
		m_bounds[i] = bounds[i];
	}
}

void iABlobManager::GetBounds( double* bounds )
{
	for ( int i = 0; i < 6; i++ )
	{
		bounds[i] = m_bounds[i];
	}
}

void iABlobManager::SetProtrusion( double protrusion )
{
	m_boundsProtrusionCoef = protrusion;
}

double iABlobManager::GetProtrusion()
{
	return m_boundsProtrusionCoef;
}

double* iABlobManager::GetBoundsProtrusion()
{
	for ( int i = 0; i < 6; i++ )
	{
		m_boundsProtrusion[i] = m_bounds[i];
	}

	double deltaBound[3];
	deltaBound[0] = ( m_boundsProtrusion[1] - m_boundsProtrusion[0] ) * ( m_boundsProtrusionCoef - 1 );
	deltaBound[1] = ( m_boundsProtrusion[3] - m_boundsProtrusion[2] ) * ( m_boundsProtrusionCoef - 1 );
	deltaBound[2] = ( m_boundsProtrusion[5] - m_boundsProtrusion[4] ) * ( m_boundsProtrusionCoef - 1 );
	m_boundsProtrusion[0] -= deltaBound[0];
	m_boundsProtrusion[1] += deltaBound[0];
	m_boundsProtrusion[2] -= deltaBound[1];
	m_boundsProtrusion[3] += deltaBound[1];
	m_boundsProtrusion[4] -= deltaBound[2];
	m_boundsProtrusion[5] += deltaBound[2];

	return m_boundsProtrusion;
}

void iABlobManager::SetDimensions( int* dimens )
{
	for ( int i = 0; i < 3; i++ )
	{
		m_dimension[i] = dimens[i];
	}
}

int* iABlobManager::GetDimensions()
{
	return m_dimension;
}

void iABlobManager::SetSmoothing( bool isOn )
{
	m_isSmoothingEnabled = isOn;
}

void iABlobManager::SetGaussianBlur( bool isOn )
{
	m_isGaussianBlurEnabled = isOn;
}

bool iABlobManager::GetSmoothing() const
{
	return m_isSmoothingEnabled;
}

bool iABlobManager::GetGaussianBlur() const
{
	return m_isGaussianBlurEnabled;
}

void iABlobManager::SetSilhouettes( bool isOn )
{
	m_isSilhoetteEnabled = isOn;
}

bool iABlobManager::GetSilhouettes() const
{
	return m_isSilhoetteEnabled;
}

void iABlobManager::SetLabeling( bool isOn )
{
	m_isLabelingEnabled = isOn;
}

bool iABlobManager::GetLabeling() const
{
	return m_isLabelingEnabled;
}

void iABlobManager::SetRenderers( vtkRenderer* blobRenderer, vtkRenderer* labelRenderer )
{
	m_blobRen = blobRenderer;
	m_labelRen = labelRenderer;

	InitRenderers();
}


void iABlobManager::InitRenderers()
{
	if ( !m_depthPeelingEnabled )
	{
		m_blobsDepthSort->SetInputConnection( m_appendedBlobsPD->GetOutputPort() );
		m_blobsDepthSort->SetDirectionToBackToFront();
		m_blobsDepthSort->SetVector( 1, 1, 1 );
		m_blobsDepthSort->SetCamera( m_blobRen->GetActiveCamera() );
		m_blobsMapper->SetInputConnection( m_blobsDepthSort->GetOutputPort() );
		m_blobsMapper->SetLookupTable( m_blobsLT );
		//m_blobsMapper->SetScalarVisibilityOn();
		m_blobsMapper->UseLookupTableScalarRangeOff();
		m_blobsMapper->SetColorModeToMapScalars();
		m_blobsActor->SetMapper( m_blobsMapper );

		m_silhouette->SetInputConnection( m_appendedBlobsPD->GetOutputPort() );
		m_silhouette->SetCamera( m_blobRen->GetActiveCamera() );
		//m_silhouetteMapper->ScalarVisibilityOff();
		m_silhouetteMapper->SetInputConnection( m_silhouette->GetOutputPort() );
		m_silhouetteActor->SetMapper( m_silhouetteMapper );

		m_blobsActor->SetVisibility( 0 );
		m_silhouetteActor->SetVisibility( 0 );
		m_blobRen->AddActor( m_blobsActor );
		m_blobRen->AddActor( m_silhouetteActor );
	}
	else
	{
		m_blobRen->RemoveActor( m_blobsActor );
		m_blobRen->RemoveActor( m_silhouetteActor );
	}
}

void iABlobManager::SetShowBlob( bool showBlob )
{
	m_isBlobBodyEnabled = showBlob;
}

void iABlobManager::SetBlobOpacity( double blobOpacity )
{
	m_blobOpacity = blobOpacity;
}

double iABlobManager::GetBlobOpacity() const
{
	return m_blobOpacity;
}

void iABlobManager::SetSilhouetteOpacity( double silhouetteOpacity )
{
	m_silhouetteOpacity = silhouetteOpacity;
}

double iABlobManager::GetSilhouetteOpacity() const
{
	return m_silhouetteOpacity;
}

void iABlobManager::SetUseDepthPeeling( bool enabled )
{
	if ( m_depthPeelingEnabled != enabled )
	{
		m_depthPeelingEnabled = enabled;
		//save to the settings
		QSettings settings;
		settings.setValue( DepthPeelingKey, m_depthPeelingEnabled );
		for ( int i = 0; i < m_blobsList.count(); i++ )
		{
			m_blobsList[i]->SetRenderIndividually( m_depthPeelingEnabled );
		}
		InitRenderers();
	}
}

bool iABlobManager::GetUseDepthPeeling() const
{
	return m_depthPeelingEnabled;
}

bool iABlobManager::GetShowBlob() const
{
	return m_isBlobBodyEnabled;
}

void iABlobManager::SaveMovie( QWidget *activeChild,
							   iARenderer * raycaster,
							   vtkCamera * cam,
							   vtkRenderWindowInteractor * /*interactor*/,
							   vtkRenderWindow * renWin,
							   size_t numberOfFrames,
							   const double /*range*/[2],
							   const double /*blobOpacity*/[2],
							   const double /*silhouetteOpacity*/[2],
							   const double /*overlapThreshold*/[2],
							   const double /*gaussianBlurVariance*/[2],
							   const int /*dimX*/[2], const int /*dimY*/[2], const int /*dimZ*/[2],
							   const QString& fileName, int mode, int qual )
{
	if ( numberOfFrames <= 1 )
		return;

	vtkSmartPointer<vtkGenericMovieWriter> movieWriter = GetMovieWriter( fileName, qual );

	if ( movieWriter.GetPointer() == nullptr )
		return;

	//interactor->Disable();

	vtkSmartPointer<vtkWindowToImageFilter> w2if = vtkSmartPointer<vtkWindowToImageFilter>::New();
	// 	int* rws = renWin->GetSize();
	// 	if (rws[0] % 2 != 0) rws[0]++;
	// 	if (rws[1] % 2 != 0) rws[1]++;
	// 	renWin->SetSize(rws);
	// 	renWin->Render();

	w2if->SetInput( renWin );
	w2if->ReadFrontBufferOff();

	movieWriter->SetInputConnection( w2if->GetOutputPort() );
	movieWriter->Start();

	//emit msg(tr("MOVIE export started. Output: %1").arg(fileName));

	//vtkSmartPointer<vtkTransform> rot = vtkSmartPointer<vtkTransform>::New();
	//double view[3];
	//double point[3];
	//if (mode == 1) { // YZ
	//	double _view[3]  = { 0 ,0, -1 };
	//	double _point[3] = { 1, 0, 0 };
	//	for (int ind=0; ind<3; ind++)
	//	{
	//		view[ind] = _view[ind];
	//		point[ind] = _point[ind];
	//	}
	//	rot->RotateZ(360/numberOfFrames);
	//} else if (mode == 2) { // XY
	//	double _view[3]  = { 0, 0, -1 };
	//	double _point[3] = { 0, 1, 0 };
	//	for (int ind=0; ind<3; ind++)
	//	{
	//		view[ind] = _view[ind];
	//		point[ind] = _point[ind];
	//	}
	//	rot->RotateX(360/numberOfFrames);
	//} else if (mode == 3) { // XZ
	//	double _view[3]  = { 0, 1, 0 };
	//	double _point[3] = { 0, 0, 1 };
	//	for (int ind=0; ind<3; ind++)
	//	{
	//		view[ind] = _view[ind];
	//		point[ind] = _point[ind];
	//	}
	//	rot->RotateY(360/numberOfFrames);
	//}
	//if(mode != 0)
	//{
	//	cam->SetFocalPoint( 0,0,0 );
	//	cam->SetViewUp ( view );
	//	cam->SetPosition ( point );
	//}
	double pscale = cam->GetParallelScale();
	double viewAngle = cam->GetViewAngle();
	raycaster->plane2()->SetNormal( 0, -1, 0 );
	for ( size_t i = 0; i < numberOfFrames; ++i )
	{
		//double t = (double)i / (numberOfFrames-1);
		//SetRange ( std::lerp(range[0], range[1], t) );
		//SetBlobOpacity ( std::lerp(blobOpacity[0], blobOpacity[1], t) );
		//SetSilhouetteOpacity ( std::lerp(silhouetteOpacity[0], silhouetteOpacity[1], t) );
		//SetOverlapThreshold ( std::lerp(overlapThreshold[0], overlapThreshold[1], t) );
		//SetGaussianBlurVariance ( std::lerp(gaussianBlurVariance[0], gaussianBlurVariance[1], t) );
		//int dims[3] = {std::lerp(dimX[0], dimX[1], t), std::lerp(dimY[0], dimY[1], t), std::lerp(dimZ[0], dimZ[1], t)};
		//SetDimensions(dims);

		//Update();

		if ( mode != 0 )
		{
			//cam->SetFocalPoint( 0,0,0 );
			m_blobRen->ResetCamera();
			cam->SetParallelScale( pscale );
			cam->SetViewAngle( viewAngle );
		}

		renWin->Render();
		w2if->Modified();

		for ( int j = 0; j < 30; ++j )
			movieWriter->Write();

		if ( movieWriter->GetError() )
		{
			//emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
			break;
		}
		//emit progress( 100 * (i+1) / (extent[1]-extent[0]));
		//QCoreApplication::processEvents();
		//if(mode != 0)
		//cam->ApplyTransform(rot);

		MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
		//mdiChild->sXZ->spinBoxXZ->setValue( i );
		//mdiChild->updateViews();

		raycaster->plane2()->SetOrigin( 0, mdiChild->imageData()->GetSpacing()[0] * ( mdiChild->imageData()->GetDimensions()[1] - i ), 0 );
		raycaster->update();
		//mdiChild->updateViews();


	}

	movieWriter->End();
	movieWriter->ReleaseDataFlagOn();
	w2if->ReleaseDataFlagOn();

	//interactor->Enable();

	if ( movieWriter->GetError() )
	{
		;//emit msg(tr("  MOVIE export failed."));
	}
	//else emit msg(tr("  MOVIE export completed."));

	return;
}
