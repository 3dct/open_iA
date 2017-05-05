/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iARegionVisModule.h"
// iA
#include "iACalculateDensityMap.h"
#include "iA4DCTVisWin.h"	// ToDo: Scale!
// vtk
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataSilhouette.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkDepthSortPolyData.h>
#include <vtkMetaImageReader.h>
#include <vtkMetaImageReader.h>
// itk
#include <itkBinaryThresholdImageFilter.h>
#include <itkResampleImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

iARegionVisModule::iARegionVisModule( )
	: iAVisModule( )
	, m_densityVal( 0.5 )
{
	setDensityMapDimension( 15, 5, 30 );

	m_regionMapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_regionMapper->ScalarVisibilityOff( );
	m_regionActor = vtkSmartPointer<vtkActor>::New( );
	m_regionActor->SetMapper( m_regionMapper );
	setSurfaceOpacity( 0.15 );

	m_silhouetteMapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_silhouetteActor = vtkSmartPointer<vtkActor>::New( );
	m_silhouetteActor->SetMapper( m_silhouetteMapper );
	setSilhoetteColor( 0, 0, 0 );
	setSurfaceColor( 0.7, 0.7, 0.7 );
	setSilhoetteOpacity( 0.25 );
	setSilhoetteLineWidth( 2.5 );
}

void iARegionVisModule::enable( )
{
	if( !isAttached( ) ) return;
	if( !isEnabled( ) ) {
		m_renderer->AddActor( m_regionActor );
		m_renderer->AddActor( m_silhouetteActor );
	}
	iAVisModule::enable( );
}

void iARegionVisModule::disable( )
{
	if( !isAttached( ) ) return;
	if( isEnabled( ) ) {
		m_renderer->RemoveActor( m_regionActor );
		m_renderer->RemoveActor( m_silhouetteActor );
	}
	iAVisModule::disable( );
}

void iARegionVisModule::setData( vtkImageData * output )
{
	// surface extraction
	vtkSmartPointer<vtkMarchingCubes> surface = vtkSmartPointer<vtkMarchingCubes>::New( );
	surface->SetInputData( output );
	surface->ComputeNormalsOn( );
	surface->SetValue( 0, 0.25 );

	vtkSmartPointer<vtkDepthSortPolyData> depthSort = vtkSmartPointer<vtkDepthSortPolyData>::New( );
	depthSort->SetInputConnection( surface->GetOutputPort( ) );
	depthSort->SetDirectionToBackToFront( );
	depthSort->SetVector( 1, 1, 1 );
	depthSort->SetCamera( m_renderer->GetActiveCamera( ) );
	depthSort->SortScalarsOn( );
	depthSort->Update( );

	m_regionMapper->SetInputConnection( depthSort->GetOutputPort( ) );

	m_silhouette = vtkSmartPointer<vtkPolyDataSilhouette>::New( );
	m_silhouette->SetInputData( surface->GetOutput( ) );
	m_silhouette->SetCamera( m_renderer->GetActiveCamera( ) );
	m_silhouette->SetEnableFeatureAngle( 1 );
	m_silhouette->SetFeatureAngle( 90 );

	m_silhouetteMapper->SetInputConnection( m_silhouette->GetOutputPort( ) );
	m_silhouetteMapper->Update( );
}

void iARegionVisModule::setPosition( double * position )
{
	// save settings
	settings.Position[0] = position[0];	settings.Position[1] = position[1];	settings.Position[2] = position[2];
	// update visualization
	m_regionActor->SetPosition( -position[0], -position[1], -position[2] );
	m_silhouetteActor->SetPosition( -position[0], -position[1], -position[2] );
}

void iARegionVisModule::setSilhoetteColor( double r, double g, double b )
{
	// save settings
	settings.SilhoetteColor = QColor( r * 255, g * 255, b * 255 );
	// update visualization
	m_silhouetteActor->GetProperty( )->SetColor( r, g, b );
}

void iARegionVisModule::setSurfaceColor( double r, double g, double b )
{
	// save settings
	settings.SurfaceColor = QColor( r * 255, g * 255, b * 255 );
	// update visualization
	m_regionActor->GetProperty( )->SetColor( r, g, b );
}

void iARegionVisModule::setSilhoetteOpacity( double opacity )
{
	// save settings
	settings.SilhoetteOpacity = opacity;
	// update visualization
	m_silhouetteActor->GetProperty( )->SetOpacity( opacity );
}

void iARegionVisModule::setSurfaceOpacity( double opacity )
{
	// save settings
	settings.SurfaceOpacity = opacity;
	// update visualization
	m_regionActor->GetProperty( )->SetOpacity( opacity );
}

void iARegionVisModule::setImage( QString fileName )
{
	//calculateDensityMap( fileName, this );

	typedef itk::Image<double, 3> DensityMapImageType;
	DensityMapImageType::Pointer image = DensityMapImageType::New( );;
	{
		typedef itk::ImageFileReader<DensityMapImageType> ReaderType;
		ReaderType::Pointer reader = ReaderType::New( );
		reader->SetFileName( fileName.toStdString( ) );
		reader->Update( );

		DensityMapImageType::IndexType index; index.Fill( 0 );
		DensityMapImageType::SizeType originalSize, size;
		originalSize = reader->GetOutput( )->GetLargestPossibleRegion( ).GetSize( );
		size[0] = originalSize[0] + 2;
		size[1] = originalSize[1] + 2;
		size[2] = originalSize[2] + 2;
		DensityMapImageType::RegionType region( index, size );
		image->SetRegions( region );
		image->SetSpacing( reader->GetOutput( )->GetSpacing( ) );
		image->Allocate( );
		image->FillBuffer( 0 );
		for( int x = 0; x < originalSize[0]; x++ )
		{
			for( int y = 0; y < originalSize[1]; y++ )
			{
				for( int z = 0; z < originalSize[2]; z++ )
				{
					DensityMapImageType::IndexType ind1, ind2;
					ind1[0] = x; ind1[1] = y; ind1[2] = z;
					ind2[0] = x + 1; ind2[1] = y + 1; ind2[2] = z + 1;
					image->SetPixel( ind2, reader->GetOutput( )->GetPixel( ind1 ) );
				}
			}
		}
	}
	DensityMapImageType::SpacingType spacing = image->GetSpacing( );
	spacing[0] = spacing[0] * SCENE_SCALE;
	spacing[1] = spacing[1] * SCENE_SCALE;
	spacing[2] = spacing[2] * SCENE_SCALE;
	image->SetSpacing( spacing );

	// binary thrshold
	typedef itk::BinaryThresholdImageFilter<DensityMapImageType, DensityMapImageType> DensitymapThresholdingType;
	DensitymapThresholdingType::Pointer densitymapThresholding = DensitymapThresholdingType::New( );
	densitymapThresholding->SetInput( image );
	densitymapThresholding->SetLowerThreshold( m_densityVal );
	densitymapThresholding->SetInsideValue( 1 );
	densitymapThresholding->SetOutsideValue( 0 );
	densitymapThresholding->Update( );

	DensityMapImageType::SizeType outputSize;
	DensityMapImageType::SizeType size = image->GetLargestPossibleRegion( ).GetSize( );
	outputSize[0] = size[0] * 4;
	outputSize[1] = size[1] * 4;
	outputSize[2] = size[2] * 4;
	//DensityMapImageType::SpacingType spacing = image->GetSpacing();
	double outputSpacing[3];
	outputSpacing[0] = spacing[0] / 4;
	outputSpacing[1] = spacing[1] / 4;
	outputSpacing[2] = spacing[2] / 4;
	typedef itk::ResampleImageFilter<DensityMapImageType, DensityMapImageType> ResampleImageFilterType;
	ResampleImageFilterType::Pointer resample = ResampleImageFilterType::New( );
	resample->SetInput( densitymapThresholding->GetOutput( ) );
	resample->SetSize( outputSize );
	resample->SetOutputSpacing( outputSpacing );
	double origin[3] = { 0, 0, 0 };
	resample->SetOutputOrigin( origin );
	resample->Update( );
	//resample->UpdateLargestPossibleRegion();

	// itk to vtk
	typedef itk::ImageToVTKImageFilter<DensityMapImageType> ConnectorType;
	ConnectorType::Pointer connector = ConnectorType::New( );
	connector->SetInput( resample->GetOutput( ) );
	connector->Update( );

	setData( connector->GetOutput( ) );
	double pos[ ] = { spacing[0], spacing[1], spacing[2] };
	setPosition( pos );
}

void iARegionVisModule::setSilhoetteLineWidth( double width )
{
	// save settings
	settings.SilhoetteWidth = width;
	// update visualization
	m_silhouetteActor->GetProperty( )->SetLineWidth( width );
}

void iARegionVisModule::setDefectDensity( double densityVal )
{
	m_densityVal = densityVal;
}

void iARegionVisModule::setDensityMapDimension( int dimX, int dimY, int dimZ )
{
	m_densityMapSize[0] = dimX;
	m_densityMapSize[1] = dimY;
	m_densityMapSize[2] = dimZ;
}

void iARegionVisModule::setDensityMapDimension( int * dim )
{
	setDensityMapDimension( dim[0], dim[1], dim[2] );
}

void iARegionVisModule::calculateDensityMap( QString fileName, iARegionVisModule* visModule )
{
	typedef itk::Image<double, 3> DoubleImageType;
	// read image
	vtkSmartPointer<vtkMetaImageReader> reader = vtkSmartPointer<vtkMetaImageReader>::New( );
	reader->SetFileName( fileName.toStdString( ).c_str( ) );
	reader->Update( );

	// calculate density map
	//int m_densityMapSize[3] = { 15, 5, 30 };
	std::vector<std::vector<std::vector<double>>> density;
	double cellSize[3];
	density = CalculateDensityMap<double, unsigned short>::Calculate( reader->GetOutput( ), m_densityMapSize, cellSize );

	// make an itk image
	double* oldSpacing = reader->GetOutput( )->GetSpacing( );
	int* extent = reader->GetOutput( )->GetExtent( );
	int imgSize[3];
	imgSize[0] = extent[1] - extent[0] + 1;
	imgSize[1] = extent[3] - extent[2] + 1;
	imgSize[2] = extent[5] - extent[4] + 1;
	DoubleImageType::IndexType index; index.Fill( 0 );
	DoubleImageType::SizeType size;
	size[0] = m_densityMapSize[0] + 2;
	size[1] = m_densityMapSize[1] + 2;
	size[2] = m_densityMapSize[2] + 2;
	DoubleImageType::RegionType region( index, size );
	DoubleImageType::Pointer image = DoubleImageType::New( );
	image->SetRegions( region );
	double newSpacing[3];
	newSpacing[0] = (double)imgSize[0] * oldSpacing[0] / ( m_densityMapSize[0] - 1 ) * SCENE_SCALE;
	newSpacing[1] = (double)imgSize[1] * oldSpacing[1] / ( m_densityMapSize[1] - 1 ) * SCENE_SCALE;
	newSpacing[2] = (double)imgSize[2] * oldSpacing[2] / ( m_densityMapSize[2] - 1 ) * SCENE_SCALE;
	image->SetSpacing( newSpacing );
	image->Allocate( );
	image->FillBuffer( 0 );
	for( int x = 0; x < density.size( ); x++ )
	{
		for( int y = 0; y < density[x].size( ); y++ )
		{
			for( int z = 0; z < density[x][y].size( ); z++ )
			{
				DoubleImageType::IndexType ind;
				ind[0] = x + 1; ind[1] = y + 1; ind[2] = z + 1;
				image->SetPixel( ind, density[x][y][z] );
			}
		}
	}

	typedef itk::ImageFileWriter<DoubleImageType> FileWriterType;
	FileWriterType::Pointer writer = FileWriterType::New( );
	writer->SetFileName( "D:\\DensityImg.mhd" );
	writer->SetInput( image );
	writer->Update( );

	// binary thrshold
	typedef itk::BinaryThresholdImageFilter<DoubleImageType, DoubleImageType> DensitymapThresholdingType;
	DensitymapThresholdingType::Pointer densitymapThresholding = DensitymapThresholdingType::New( );
	densitymapThresholding->SetInput( image );
	densitymapThresholding->SetLowerThreshold( m_densityVal * cellSize[0] * cellSize[1] * cellSize[2] );
	densitymapThresholding->SetInsideValue( 1 );
	densitymapThresholding->SetOutsideValue( 0 );
	densitymapThresholding->Update( );

	DoubleImageType::SizeType outputSize;
	outputSize[0] = size[0] * 4;
	outputSize[1] = size[1] * 4;
	outputSize[2] = size[2] * 4;
	double outputSpacing[3];
	outputSpacing[0] = newSpacing[0] / 4;
	outputSpacing[1] = newSpacing[1] / 4;
	outputSpacing[2] = newSpacing[2] / 4;
	typedef itk::ResampleImageFilter<DoubleImageType, DoubleImageType> ResampleImageFilterType;
	ResampleImageFilterType::Pointer resample = ResampleImageFilterType::New( );
	resample->SetInput( densitymapThresholding->GetOutput( ) );
	resample->SetSize( outputSize );
	resample->SetOutputSpacing( outputSpacing );
	//resample->UpdateLargestPossibleRegion();

	// itk to vtk
	typedef itk::ImageToVTKImageFilter<DoubleImageType> ConnectorType;
	ConnectorType::Pointer connector = ConnectorType::New( );
	connector->SetInput( resample->GetOutput( ) );
	connector->Update( );

	setData( connector->GetOutput( ) );
	setPosition( newSpacing );
}