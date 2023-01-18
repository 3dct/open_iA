/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAITKIO.h"

#include "iAFileUtils.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include "iAExtendedTypedCallHelper.h"

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOFactory.h>

#include <QString>

namespace iAITKIO
{

template <class T>
void read_image_template(QString const& f, ImagePointer& image, bool releaseFlag, iAProgress const * progress)
{
	typedef itk::Image<T, Dim> InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();
	if (releaseFlag)
	{
		reader->ReleaseDataFlagOn();
	}
	if (progress)
	{
		progress->observe(reader);
	}
	reader->SetFileName(getLocalEncodingFileName(f));
	reader->Update();
	image = reader->GetOutput();
	image->Modified();
}

template <class T>
void write_image_template(bool comp, QString const& fileName, ImagePtr image, iAProgress const * progress)
{
	using InputImageType = itk::Image<T, Dim>;
	auto writer = itk::ImageFileWriter<InputImageType>::New();

	writer->ReleaseDataFlagOn();
	std::string encodedFileName = getLocalEncodingFileName(fileName);
	if (encodedFileName.empty())
	{
		return;
	}
	if (progress)
	{
		progress->observe(writer);
	}
	writer->SetFileName(encodedFileName.c_str());
	writer->SetInput(dynamic_cast<InputImageType*>(image));
	writer->SetUseCompression(comp);
	writer->Update();
}

ImagePointer readFile(QString const& fileName, PixelType& pixelType, ScalarType& scalarType, bool releaseFlag, iAProgress const * progress)
{
	auto imageIO = itk::ImageIOFactory::CreateImageIO(getLocalEncodingFileName(fileName).c_str(), itk::ImageIOFactory::ReadMode);

	if (!imageIO)
	{
		throw itk::ExceptionObject(__FILE__, __LINE__,
			QString("Could not open file %1: ITK does not have a reader that can handle this type of file!").arg(fileName).toStdString().c_str());
	}

	imageIO->SetFileName(getLocalEncodingFileName(fileName));
	imageIO->ReadImageInformation();
	scalarType = imageIO->GetComponentType();
	pixelType  = imageIO->GetPixelType();
	ImagePointer image;
	ITK_EXTENDED_TYPED_CALL(read_image_template, scalarType, pixelType, fileName, image, releaseFlag, progress);

	return image;
}

void writeFile(QString const& fileName, ImagePtr image, ScalarType scalarType, bool useCompression, iAProgress const * progress)
{
	ITK_TYPED_CALL(write_image_template, scalarType, useCompression, fileName, image, progress);
}
}  // namespace iAITKIO
