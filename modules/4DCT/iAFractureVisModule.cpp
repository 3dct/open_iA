// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFractureVisModule.h"

#include "iA4DCTVisWin.h"

#include <iAFileUtils.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkDiscreteGaussianImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkMinimumMaximumImageCalculator.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkDepthSortPolyData.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkTriangle.h>

#include <QVector>

double interpolate( double val1, double val2, double coeff )
{
	return ( 1 - coeff ) * val1 + coeff * val2;
}

iAFractureVisModule::iAFractureVisModule( )
	: iAVisModule( )
	, Step( 3 )
	, m_lowIntensity( Qt::blue )
	, m_highIntensity( Qt::red )
{
	m_surfMapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_surfActor = vtkSmartPointer<vtkActor>::New( );
	m_surfActor->SetMapper( m_surfMapper );
	//m_surfActor->GetProperty()->SetOpacity(0.5);
}

void iAFractureVisModule::show( )
{
	m_surfActor->GetProperty( )->SetInterpolationToPhong( );
	m_renderer->AddActor( m_surfActor );
}

void iAFractureVisModule::hide( )
{
	m_renderer->RemoveActor( m_surfActor );
}

void iAFractureVisModule::load( QString fileName )
{
	// load heightmap from the input file
	typedef itk::ImageFileReader<MapType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New( );
	reader->SetFileName( getLocalEncodingFileName(fileName) );
	reader->Update( );
	m_heightmap = reader->GetOutput( );
	// visualize
	m_points = vtkSmartPointer<vtkPoints>::New( );
	m_polys = vtkSmartPointer<vtkCellArray>::New( );
	calculatePoints( m_points, m_heightmap, m_size );
	calculatePolys( m_polys, m_heightmap );
	setData( m_points, m_polys );
	//m_renderer->Render();
}

void iAFractureVisModule::save( QString fileName )
{
	typedef itk::ImageFileWriter<MapType> WriterType;
	WriterType::Pointer writer = WriterType::New( );
	writer->SetInput( m_heightmap );
	writer->SetFileName( getLocalEncodingFileName(fileName) );
	writer->Update( );
}

void iAFractureVisModule::extract( QString fileName )
{
	// create a new heightmap and calculate it
	m_heightmap = MapType::New( );
	calculateMap( m_heightmap, fileName, MapName::Heightmap );
	// visualize
	m_points = vtkSmartPointer<vtkPoints>::New( );
	m_polys = vtkSmartPointer<vtkCellArray>::New( );
	calculatePoints( m_points, m_heightmap, m_size );
	calculatePolys( m_polys, m_heightmap );
	setData( m_points, m_polys, m_colors );
}

void iAFractureVisModule::setSize( double * size )
{
	m_size[0] = size[0]; m_size[1] = size[1]; m_size[2] = size[2];
}

void iAFractureVisModule::setColorMap( QString fileName )
{
	m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New( );
	m_colors->SetNumberOfComponents( 3 );
	m_colors->SetName( "Colors" );
	m_colormap = MapType::New( );
	calculateMap( m_colormap, fileName, MapName::Colormap );
	calculateColors( m_colors, m_colormap, m_heightmap );
	setData( m_points, m_polys, m_colors );
}

void iAFractureVisModule::setLowIntensityColor( QColor color )
{
	m_lowIntensity = color;
	m_colors->Reset( );
	calculateColors( m_colors, m_colormap, m_heightmap );
	setData( m_points, m_polys, m_colors );
}

void iAFractureVisModule::setHighIntensityColor( QColor color )
{
	m_highIntensity = color;
	m_colors->Reset( );
	calculateColors( m_colors, m_colormap, m_heightmap );
	setData( m_points, m_polys, m_colors );
}

void iAFractureVisModule::setColor( QColor color )
{
	m_surfActor->GetProperty( )->SetColor( color.redF( ), color.greenF( ), color.blueF( ) );
}

void iAFractureVisModule::setAmbient( double coeff )
{
	m_surfActor->GetProperty( )->SetAmbient( coeff );
	//emit updateRe
}

void iAFractureVisModule::setOpacity( double val )
{
	m_surfActor->GetProperty( )->SetOpacity( val );
}

void iAFractureVisModule::getBounds( double * bounds )
{
	m_surfMapper->GetBounds( bounds );
}

void iAFractureVisModule::setData( vtkPoints* points, vtkCellArray* polys, vtkUnsignedCharArray* colors /*= 0*/ )
{
	vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New( );
	polyData->SetPoints( points );
	polyData->SetPolys( polys );
	if( colors != nullptr )
	{
		polyData->GetPointData( )->SetScalars( colors );
		m_surfMapper->SetScalarVisibility( 1 );
	}
	else
	{
		m_surfMapper->SetScalarVisibility( 0 );
	}
	vtkSmartPointer<vtkDepthSortPolyData> depthSort = vtkSmartPointer<vtkDepthSortPolyData>::New( );
	depthSort->SetInputData( polyData );
	depthSort->SetDirectionToBackToFront( );
	depthSort->SetVector( 1, 1, 1 );
	depthSort->SetCamera( m_renderer->GetActiveCamera( ) );
	depthSort->SortScalarsOn( );
	depthSort->Update( );
	m_surfMapper->SetInputConnection( depthSort->GetOutputPort( ) );
	m_surfMapper->Update( );
}

void iAFractureVisModule::calculateMap( MapType* map, QString fileName, MapName mapName )
{
	typedef unsigned short							PixelType;
	typedef itk::Image<PixelType, 3>				ImageType;
	typedef itk::ImageFileReader<ImageType>			ReaderType;

	ReaderType::Pointer reader = ReaderType::New( );
	reader->SetFileName( getLocalEncodingFileName(fileName) );
	reader->Update( );

	ImageType::Pointer image = reader->GetOutput( );
	ImageType::SizeType imgSize = image->GetLargestPossibleRegion( ).GetSize( );

	int minZ = 0.5 * imgSize[2];
	int maxZ = 1. * imgSize[2];

	//HeightmapType::Pointer heightmap = HeightmapType::New();
	//map = MapType::New();
	itk::Index<2> startInd; startInd.Fill( 0 );
	itk::Size<2> size; size[0] = imgSize[0]; size[1] = imgSize[1];
	MapType::RegionType region( startInd, size );
	map->SetRegions( region );
	map->Allocate( );
	map->FillBuffer( 0 );

	//double previousValue = (double)(minZ + maxZ) / 2;
	double previousValue = 0.5;
	ImageType::IndexType ind;
	assert(imgSize[0] <= static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()));
	for( ind[0] = 0u; static_cast<itk::SizeValueType>(ind[0]) < imgSize[0]; ind[0]++ )
	{
		for( ind[1] = 0u; static_cast<itk::SizeValueType>(ind[1]) < imgSize[1]; ind[1]++ )
		{
			QVector<unsigned int> ray;
			//for (ind[2] = 0; ind[2] < imgSize[2]; ind[2]++) {
			for( ind[2] = minZ; ind[2] < maxZ; ind[2]++ )
			{
				PixelType pixel = image->GetPixel( ind );
				if( pixel > 0 )
				{
					ray.push_back( static_cast<uint>(ind[2]) );
				}
			}

			double value;
			switch( mapName )
			{
			case MapName::Heightmap:
				value = 0;
				for( auto r : ray )
				{
					value += r;
				}
				if( ray.size( ) > 0 )
				{
					value /= ( ray.size( ) * imgSize[2] );
					previousValue = value;
				}
				else
				{
					value = previousValue;
				}
				break;
			case MapName::Colormap:
				value = ray.size( );
				break;
			default:
				value = -1;
				break;
			}

			MapType::IndexType index; index[0] = ind[0]; index[1] = ind[1];
			map->SetPixel( index, value );
		}
	}
}

void iAFractureVisModule::calculatePoints( vtkPoints* points, MapType* heightmap, double * size )
{
	//m_points = vtkSmartPointer<vtkPoints>::New();

	// Create and setup a Gaussian filter
	typedef itk::DiscreteGaussianImageFilter<MapType, MapType> GaussianImageFilterType;
	GaussianImageFilterType::Pointer gaussianFilter = GaussianImageFilterType::New( );
	gaussianFilter->SetInput( heightmap );
	gaussianFilter->SetVariance( 200.0 );
	gaussianFilter->SetMaximumKernelWidth( 64 );
	gaussianFilter->Update( );
	MapType::Pointer bluredHeightmap = gaussianFilter->GetOutput( );

	MapType::SizeType heightmapSize = bluredHeightmap->GetLargestPossibleRegion( ).GetSize( );
	MapType::IndexType heightmapInd;

	auto xSteps = heightmapSize[0] / Step;
	auto ySteps = heightmapSize[1] / Step;

	for(uint64_t i = 0; i < xSteps; i++ )
	{
		for(uint64_t j = 0; j < ySteps; j++ )
		{
			heightmapInd[0] = i * Step;
			heightmapInd[1] = j * Step;
			double val = bluredHeightmap->GetPixel( heightmapInd );
			double pos[3];
			pos[0] = size[0] * heightmapInd[0] / heightmapSize[0] * SCENE_SCALE;
			pos[1] = size[1] * heightmapInd[1] / heightmapSize[1] * SCENE_SCALE;
			pos[2] = size[2] * val * SCENE_SCALE;
			points->InsertNextPoint( pos[0], pos[1], pos[2] );
		}
	}
}

void iAFractureVisModule::calculatePolys( vtkCellArray* polys, MapType* heightmap )
{
	//m_mesh = vtkSmartPointer<vtkCellArray>::New();

	MapType::IndexType heightmapInd;
	MapType::SizeType heightmapSize = heightmap->GetLargestPossibleRegion( ).GetSize( );

	auto xSteps = heightmapSize[0] / Step;
	auto ySteps = heightmapSize[1] / Step;
	int skip[3] = { 60, 30 };

	for (vtkIdType i = skip[0]; i < static_cast<vtkIdType>(xSteps - 1 - skip[0]); ++i )
	{
		for (vtkIdType j = skip[1]; j < static_cast<vtkIdType>(ySteps - 1 - skip[1]); ++j )
		{
			heightmapInd[0] = i * Step; heightmapInd[1] = j * Step;

			vtkIdType ind[4];
			ind[0] = i      * ySteps + j;
			ind[1] = i      * ySteps + j + 1;
			ind[2] = ( i + 1 ) * ySteps + j;
			ind[3] = ( i + 1 ) * ySteps + j + 1;

			vtkSmartPointer<vtkTriangle> triangle[2];
			triangle[0] = vtkSmartPointer<vtkTriangle>::New( );
			triangle[1] = vtkSmartPointer<vtkTriangle>::New( );
			triangle[0]->GetPointIds( )->SetId( 0, ind[0] );
			triangle[0]->GetPointIds( )->SetId( 1, ind[1] );
			triangle[0]->GetPointIds( )->SetId( 2, ind[2] );
			triangle[1]->GetPointIds( )->SetId( 0, ind[3] );
			triangle[1]->GetPointIds( )->SetId( 1, ind[1] );
			triangle[1]->GetPointIds( )->SetId( 2, ind[2] );
			polys->InsertNextCell( triangle[0] );
			polys->InsertNextCell( triangle[1] );
		}
	}
}

void iAFractureVisModule::calculateColors( vtkUnsignedCharArray* colors, MapType* colormap, MapType* heightmap )
{
	//m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();

	MapType::SizeType heightmapSize = heightmap->GetLargestPossibleRegion( ).GetSize( );
	MapType::SizeType colormapSize = colormap->GetLargestPossibleRegion( ).GetSize( );
	MapType::IndexType heightmapInd, colormapInd;

	auto xSteps = heightmapSize[0] / Step;
	auto ySteps = heightmapSize[1] / Step;

	typedef itk::MinimumMaximumImageCalculator<MapType> ImageCalculatorFilterType;
	ImageCalculatorFilterType::Pointer imageCalcFilter = ImageCalculatorFilterType::New( );
	imageCalcFilter->SetImage( colormap );
	imageCalcFilter->Compute( );
	double colorRange = imageCalcFilter->GetMaximum( ) - imageCalcFilter->GetMinimum( );

	for(uint64_t i = 0; i < xSteps; ++i)
	{
		for(uint64_t j = 0; j < ySteps; ++j)
		{
			heightmapInd[0] = i * Step;
			heightmapInd[1] = j * Step;
			unsigned char col[3];
			colormapInd[0] = static_cast<double>(heightmapInd[0]) / heightmapSize[0] * colormapSize[0];
			colormapInd[1] = static_cast<double>(heightmapInd[1]) / heightmapSize[1] * colormapSize[1];
			double colorVal = colormap->GetPixel( colormapInd );
			double coeff = colorVal / colorRange;
			col[0] = interpolate( m_lowIntensity.red( ), m_highIntensity.red( ), coeff );
			col[1] = interpolate( m_lowIntensity.green( ), m_highIntensity.green( ), coeff );
			col[2] = interpolate( m_lowIntensity.blue( ), m_highIntensity.blue( ), coeff );
			colors->InsertNextTypedTuple( col );
		}
	}
}
