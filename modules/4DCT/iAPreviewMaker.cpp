// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPreviewMaker.h"

#include <iALog.h>
#include <iAITKIO.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkExtractImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkImageIOBase.h>

void iAPreviewMaker::makeUsingType( QString fileName, QString thumbFileName )
{
	itk::ImageIOBase::Pointer imageIO;
	imageIO = itk::ImageIOFactory::CreateImageIO( fileName.toStdString().c_str( ), itk::ImageIOFactory::ReadMode );
	if( !imageIO )
	{
		//std::cerr << "Could not CreateImageIO for: " << inputFilename << std::endl;
		return;
	}
	imageIO->SetFileName( fileName.toStdString() );
	imageIO->ReadImageInformation( );

	auto pixelType = imageIO->GetComponentType( );
	switch( pixelType )
	{
	case iAITKIO::ScalarType::USHORT:
	{
		typedef unsigned short PixelType;
		makeUsingType<PixelType>( fileName, thumbFileName );
		break;
	}
	case iAITKIO::ScalarType::ULONG:
	{
		typedef unsigned long int PixelType;
		makeUsingType<PixelType>( fileName, thumbFileName );
		break;
	}
	default:
		LOG(lvlWarn, "Image pixel type not supported!");
		break;
	}
}

template<typename TPixelType>
void iAPreviewMaker::makeUsingType( QString filename, QString thumbFileName )
{
	typedef itk::Image<TPixelType, 3> InputImageType;
	typedef itk::Image<TPixelType, 2> OutputImageType;

	// read image
	auto reader = itk::ImageFileReader<InputImageType>::New();
	reader->SetFileName( filename.toStdString() );
	reader->Update( );
	typename InputImageType::Pointer image = reader->GetOutput( );

	// extract the region
	auto inputSize = image->GetLargestPossibleRegion( ).GetSize( );
	typename InputImageType::IndexType desiredStart;
	desiredStart = {{ 0, static_cast<typename InputImageType::IndexType::IndexValueType>( inputSize[1] ) / 2, 0 }};
	typename InputImageType::SizeType desiredSize;
	desiredSize = {{ inputSize[0], 0, inputSize[2] }};
	typename InputImageType::RegionType desiredReg( desiredStart, desiredSize );

	auto filter = itk::ExtractImageFilter<InputImageType, OutputImageType>::New();
	filter->SetExtractionRegion( desiredReg );
	filter->SetInput( image );
	filter->SetDirectionCollapseToIdentity( );
	filter->Update( );
	typename OutputImageType::Pointer output = filter->GetOutput( );
	output->DisconnectPipeline( );


	// write
	auto writer = itk::ImageFileWriter<OutputImageType>::New();
	writer->SetFileName( thumbFileName.toStdString() );
	writer->SetInput( output );
	writer->Update( );
}
