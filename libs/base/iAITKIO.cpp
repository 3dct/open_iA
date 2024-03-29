// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAITKIO.h"

#include "iAFileUtils.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include "iAExtendedTypedCallHelper.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOFactory.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <QString>

namespace iAITKIO
{

template <class T>
void read_image_template(QString const& f, ImagePointer& image, bool releaseFlag, iAProgress const * progress)
{
	auto reader = itk::ImageFileReader<itk::Image<T, Dim>>::New();
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
