// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARegionVisModule.h"

#include "iACalculateDensityMap.h"
#include "iA4DCTVisWin.h"	// ToDo: Scale!

#include <iAToolsVTK.h>

#include <itkBinaryThresholdImageFilter.h>
#include <itkResampleImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

#include <vtkActor.h>
#include <vtkDepthSortPolyData.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataSilhouette.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include <cassert>

iARegionVisModule::iARegionVisModule( )
	: iAVisModule( )
	, m_densityVal( 0.5 )
{
	setDensityMapDimension( 15, 5, 30 );

	m_regionMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_regionMapper->ScalarVisibilityOff( );
	m_regionActor = vtkSmartPointer<vtkActor>::New();
	m_regionActor->SetMapper( m_regionMapper );
	setSurfaceOpacity( 0.15 );

	m_silhouetteMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_silhouetteActor = vtkSmartPointer<vtkActor>::New();
	m_silhouetteActor->SetMapper( m_silhouetteMapper );
	setSilhoetteColor( 0, 0, 0 );
	setSurfaceColor( 0.7, 0.7, 0.7 );
	setSilhoetteOpacity( 0.25 );
	setSilhoetteLineWidth( 2.5 );
}

void iARegionVisModule::show( )
{
	m_renderer->AddActor( m_regionActor );
	m_renderer->AddActor( m_silhouetteActor );
}

void iARegionVisModule::hide( )
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
	auto surface = vtkSmartPointer<vtkMarchingCubes>::New();
	surface->SetInputData( output );
	surface->ComputeNormalsOn( );
	surface->SetValue( 0, 0.25 );

	auto depthSort = vtkSmartPointer<vtkDepthSortPolyData>::New();
	depthSort->SetInputConnection( surface->GetOutputPort( ) );
	depthSort->SetDirectionToBackToFront( );
	depthSort->SetVector( 1, 1, 1 );
	depthSort->SetCamera( m_renderer->GetActiveCamera( ) );
	depthSort->SortScalarsOn( );
	depthSort->Update( );

	m_regionMapper->SetInputConnection( depthSort->GetOutputPort( ) );

	m_silhouette = vtkSmartPointer<vtkPolyDataSilhouette>::New();
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
	auto image = DensityMapImageType::New();
	{
		auto reader = itk::ImageFileReader<DensityMapImageType>::New();
		reader->SetFileName( fileName.toStdString() );
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
		DensityMapImageType::IndexType ind1;
		assert(originalSize[0] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
			originalSize[1] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
			originalSize[2] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()));
		for(ind1[0] = 0; static_cast<itk::SizeValueType>(ind1[0]) < originalSize[0]; ++ind1[0])
		{
			for(ind1[1] = 0; static_cast<itk::SizeValueType>(ind1[1]) < originalSize[1]; ++ind1[1])
			{
				for(ind1[2] = 0; static_cast<itk::SizeValueType>(ind1[2]) < originalSize[2]; ++ind1[2])
				{
					DensityMapImageType::IndexType ind2;
					ind2[0] = ind1[0] + 1; ind2[1] = ind1[1] + 1; ind2[2] = ind1[2] + 1;
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
	auto densitymapThresholding = itk::BinaryThresholdImageFilter<DensityMapImageType, DensityMapImageType>::New();
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
	auto resample = itk::ResampleImageFilter<DensityMapImageType, DensityMapImageType>::New();
	resample->SetInput( densitymapThresholding->GetOutput( ) );
	resample->SetSize( outputSize );
	resample->SetOutputSpacing( outputSpacing );
	double origin[3] = { 0, 0, 0 };
	resample->SetOutputOrigin( origin );
	resample->Update( );
	//resample->UpdateLargestPossibleRegion();

	// itk to vtk
	typedef itk::ImageToVTKImageFilter<DensityMapImageType> ConnectorType;
	auto connector = ConnectorType::New();
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

void iARegionVisModule::calculateDensityMap( QString fileName, iARegionVisModule* /*visModule*/ )
{
	typedef itk::Image<double, 3> DoubleImageType;
	// read image
	auto img = readImage(fileName);

	// calculate density map
	//int m_densityMapSize[3] = { 15, 5, 30 };
	std::vector<std::vector<std::vector<double>>> density;
	double cellSize[3];
	density = CalculateDensityMap<double, unsigned short>::Calculate(img, m_densityMapSize, cellSize );

	// make an itk image
	double* oldSpacing = img->GetSpacing( );
	int* extent = img->GetExtent( );
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
	auto image = DoubleImageType::New();
	image->SetRegions( region );
	double newSpacing[3];
	newSpacing[0] = (double)imgSize[0] * oldSpacing[0] / ( m_densityMapSize[0] - 1 ) * SCENE_SCALE;
	newSpacing[1] = (double)imgSize[1] * oldSpacing[1] / ( m_densityMapSize[1] - 1 ) * SCENE_SCALE;
	newSpacing[2] = (double)imgSize[2] * oldSpacing[2] / ( m_densityMapSize[2] - 1 ) * SCENE_SCALE;
	image->SetSpacing( newSpacing );
	image->Allocate( );
	image->FillBuffer( 0 );
	for (size_t x = 0; x < density.size(); ++x)
	{
		for (size_t y = 0; y < density[x].size(); ++y)
		{
			for (size_t z = 0; z < density[x][y].size(); ++z)
			{
				assert(x < static_cast<size_t>(std::numeric_limits<itk::IndexValueType>::max()) &&
				       y < static_cast<size_t>(std::numeric_limits<itk::IndexValueType>::max()) &&
				       z < static_cast<size_t>(std::numeric_limits<itk::IndexValueType>::max()));
				DoubleImageType::IndexType ind;
				ind[0] = x + 1; ind[1] = y + 1; ind[2] = z + 1;
				image->SetPixel( ind, density[x][y][z] );
			}
		}
	}

	typedef itk::ImageFileWriter<DoubleImageType> FileWriterType;
	auto writer = FileWriterType::New();
	writer->SetFileName( "D:\\DensityImg.mhd" );
	writer->SetInput( image );
	writer->Update( );

	// binary thrshold
	auto densitymapThresholding = itk::BinaryThresholdImageFilter<DoubleImageType, DoubleImageType>::New();
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
	auto resample = itk::ResampleImageFilter<DoubleImageType, DoubleImageType>::New();
	resample->SetInput( densitymapThresholding->GetOutput( ) );
	resample->SetSize( outputSize );
	resample->SetOutputSpacing( outputSpacing );
	//resample->UpdateLargestPossibleRegion();

	// itk to vtk
	auto connector = itk::ImageToVTKImageFilter<DoubleImageType>::New();
	connector->SetInput( resample->GetOutput( ) );
	connector->Update( );

	setData( connector->GetOutput( ) );
	setPosition( newSpacing );
}
