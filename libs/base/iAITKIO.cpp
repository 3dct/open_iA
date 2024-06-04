// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAITKIO.h"

#include "iALog.h"
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

#include <QFileInfo>
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
	reader->SetFileName(f.toStdString());
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
	if (fileName.isEmpty())
	{
		return;
	}
	if (progress)
	{
		progress->observe(writer);
	}
	writer->SetFileName(fileName.toStdString());
	writer->SetInput(dynamic_cast<InputImageType*>(image));
	writer->SetUseCompression(comp);
	writer->Update();
}

ImagePointer readFile(QString const& fileName, PixelType& pixelType, ScalarType& scalarType, bool releaseFlag, iAProgress const * progress)
{
	auto imageIO = itk::ImageIOFactory::CreateImageIO(fileName.toStdString().c_str(), itk::ImageIOFactory::ReadMode);

	if (!imageIO)
	{
		throw itk::ExceptionObject(__FILE__, __LINE__,
			QString("Could not open file %1: ITK does not have a reader that can handle this type of file!").arg(fileName).toStdString().c_str());
	}

	imageIO->SetFileName(fileName.toStdString());
	imageIO->ReadImageInformation();
	scalarType = imageIO->GetComponentType();
	pixelType  = imageIO->GetPixelType();
	ImagePointer image;
#ifdef _MSC_VER
	try
	{
#endif
		ITK_EXTENDED_TYPED_CALL(read_image_template, scalarType, pixelType, fileName, image, releaseFlag, progress);
#ifdef _MSC_VER
	}
	catch (std::exception& e)
	{
		if (!fileName.endsWith(".mhd"))
		{
			throw e;
		}
		// potentially, the file encoding is the problem (used to be ansi, now utf-8)
		QFileInfo fi(fileName);
		QString testUtf8Filename = fi.canonicalPath() + "/" + fi.completeBaseName() + "-utf8.mhd";
		if (QFile::exists(testUtf8Filename))
		{
			LOG(lvlWarn, QString("File %1 was not readable; assuming it had the wrong encoding - a replacement utf-8 encoded .mhd file exists, trying to load that: %2")
				.arg(fileName).arg(testUtf8Filename));
		}
		else
		{   // if -utf8 file doesn't exist yet, create by re-encoding the file from Ansi (Latin1?):
			QFile inFile(fileName);
			if (!inFile.open(QFile::ReadOnly | QFile::Text))
			{
				LOG(lvlWarn, QString("In an effort to check whether .mhd encoding was the problem - failed to open '%1' for reading!").arg(fileName));
				throw e;
			}
			QTextStream inStream(&inFile);
			inStream.setEncoding(QStringConverter::Latin1);
			QString text = inStream.readAll();

			QFile outFile(testUtf8Filename);
			if (!outFile.open(QFile::WriteOnly | QFile::Text))
			{
				LOG(lvlWarn, QString("In an effort to check whether .mhd encoding was the problem - failed to open '%1' for writing!").arg(testUtf8Filename));
				throw e;
			}
			QTextStream outStream(&outFile);
			outStream << text;
			outFile.close();
			LOG(lvlWarn, QString("File %1 was not readable; assuming it had the wrong encoding - creating a replacement utf-8 encoded .mhd file and trying to load that: %2")
				.arg(fileName).arg(testUtf8Filename));
		}
		ITK_EXTENDED_TYPED_CALL(read_image_template, scalarType, pixelType, testUtf8Filename, image, releaseFlag, progress);
	}
#endif

	return image;
}

void writeFile(QString const& fileName, ImagePtr image, ScalarType scalarType, bool useCompression, iAProgress const * progress)
{
	ITK_TYPED_CALL(write_image_template, scalarType, useCompression, fileName, image, progress);
}
}  // namespace iAITKIO
