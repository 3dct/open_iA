/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAPreviewMaker.h"

#include <io/iAFileUtils.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkExtractImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkImageIORegion.h>
#include <itkImageIOBase.h>

void iAPreviewMaker::makeUsingType( QString fileName, QString thumbFileName )
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

	itk::ImageIOBase::Pointer imageIO;
	imageIO = itk::ImageIOFactory::CreateImageIO( getLocalEncodingFileName(fileName).c_str( ), itk::ImageIOFactory::ReadMode );
	if( !imageIO ) {
		//std::cerr << "Could not CreateImageIO for: " << inputFilename << std::endl;
		return;
	}
	imageIO->SetFileName( getLocalEncodingFileName(fileName) );
	imageIO->ReadImageInformation( );

	const ScalarPixelType pixelType = imageIO->GetComponentType( );
	switch( pixelType )
	{
	case itk::ImageIOBase::USHORT:
	{
		typedef unsigned short PixelType;
		makeUsingType<PixelType>( fileName, thumbFileName );
		break;
	}
	case itk::ImageIOBase::ULONG:
	{
		typedef unsigned long int PixelType;
		makeUsingType<PixelType>( fileName, thumbFileName );
		break;
	}
	}
}

template<typename TPixelType>
void iAPreviewMaker::makeUsingType( QString filename, QString thumbFileName )
{
	typedef itk::Image<TPixelType, 3> InputImageType;
	typedef itk::Image<TPixelType, 2> OutputImageType;

	// read image
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New( );

	reader->SetFileName( getLocalEncodingFileName(filename) );
	reader->Update( );
	typename InputImageType::Pointer image = reader->GetOutput( );


	// extract the region
	typename InputImageType::SizeType inputSize = image->GetLargestPossibleRegion( ).GetSize( );
	typename InputImageType::IndexType desiredStart;
	desiredStart = {{ 0, static_cast<typename InputImageType::IndexType::IndexValueType>( inputSize[1] ) / 2, 0 }};
	typename InputImageType::SizeType desiredSize;
	desiredSize = {{ inputSize[0], 0, inputSize[2] }};
	typename InputImageType::RegionType desiredReg( desiredStart, desiredSize );

	typedef itk::ExtractImageFilter<InputImageType, OutputImageType> FilterType;
	typename FilterType::Pointer filter = FilterType::New( );
	filter->SetExtractionRegion( desiredReg );
	filter->SetInput( image );
	filter->SetDirectionCollapseToIdentity( );
	filter->Update( );
	typename OutputImageType::Pointer output = filter->GetOutput( );
	output->DisconnectPipeline( );


	// write
	typedef itk::ImageFileWriter<OutputImageType> WriterType;
	typename WriterType::Pointer writer = WriterType::New( );
	writer->SetFileName( getLocalEncodingFileName(thumbFileName) );
	writer->SetInput( output );
	writer->Update( );
}
