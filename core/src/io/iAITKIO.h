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
#pragma once

#include "iATypedCallHelper.h"

#include "io/iAFileUtils.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>
#include <itkImageIOFactory.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

#include <QString>

namespace iAITKIO
{
	// type definitions - unify with iAITKIO definitions, and with defines.h DIM!
	static const int m_DIM = 3;
	typedef itk::ImageBase< m_DIM > ImageBaseType;
	typedef ImageBaseType::Pointer ImagePointer;
	typedef ImageBaseType* ImagePtr;
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

	template<class T>
	inline void read_image_template( QString const & f, ImagePointer & image, bool releaseFlag )
	{
		typedef itk::Image< T, m_DIM>   InputImageType;
		typedef itk::ImageFileReader<InputImageType> ReaderType;
		typename ReaderType::Pointer reader = ReaderType::New();

		if (releaseFlag)
		{
			reader->ReleaseDataFlagOn();
		}
		reader->SetFileName( getLocalEncodingFileName(f) );
		reader->Update();
		image = reader->GetOutput();
		image->Modified();
	}

	template<class T>
	inline void write_image_template( bool comp, QString const & fileName, ImagePtr image )
	{
		typedef itk::Image< T, m_DIM>   InputImageType;
		typedef itk::ImageFileWriter<InputImageType> WriterType;
		typename WriterType::Pointer writer = WriterType::New();

		writer->ReleaseDataFlagOn();
		std::string encodedFileName = getLocalEncodingFileName(fileName);
		if (encodedFileName.empty())
		{
			return;
		}
		writer->SetFileName( encodedFileName.c_str() );
		writer->SetInput( dynamic_cast<InputImageType *> (image) );
		writer->SetUseCompression( comp );
		writer->Update();
	}

	// TODO: unify with mdichild::loadfile / iAIO!
	inline ImagePointer readFile (QString const & fileName, ScalarPixelType & pixelType, bool releaseFlag)
	{
		itk::ImageIOBase::Pointer imageIO =
		itk::ImageIOFactory::CreateImageIO( getLocalEncodingFileName(fileName).c_str(), itk::ImageIOFactory::ReadMode );

		if (!imageIO)
		{
			throw itk::ExceptionObject( __FILE__, __LINE__, QString("iAITKIO: Could not open file %1, aborting loading.").arg(fileName).toStdString().c_str() );
			//return ImagePointer();
		}

		imageIO->SetFileName( getLocalEncodingFileName(fileName) );
		imageIO->ReadImageInformation();
		pixelType = imageIO->GetComponentType();
		ImagePointer image;
		ITK_TYPED_CALL( read_image_template, pixelType, fileName, image, releaseFlag);

		return image;
	}

	inline void writeFile (QString const & fileName, ImagePtr image, ScalarPixelType pixelType, bool useCompression = false )
	{
		ITK_TYPED_CALL(write_image_template, pixelType, useCompression, fileName, image);
	}
} // namespace iAITKIO
