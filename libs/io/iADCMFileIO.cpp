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
#include "iADCMFileIO.h"

#include "iAFileUtils.h"   // for getLocalEncodingFileName
#include "iAITKIO.h"       // for iAITKIO::Dim
#include "iAProgress.h"
#include "iAToolsITK.h"
#include "iATypedCallHelper.h"

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>

#include <vtkImageData.h>

#include <QFileInfo>


const QString iADCMFileIO::Name("DICOM files");

iADCMFileIO::iADCMFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{}

std::shared_ptr<iADataSet> iADCMFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	using PixelType = signed short; // check why signed short
	using ImageType =  itk::Image<PixelType, iAITKIO::Dim>;
	auto reader = itk::ImageSeriesReader<ImageType>::New();
	auto dicomIO = itk::GDCMImageIO::New();
	reader->SetImageIO(dicomIO);
	progress.observe(reader);
	auto nameGenerator = itk::GDCMSeriesFileNames::New();
	nameGenerator->SetUseSeriesDetails(true);
	nameGenerator->SetDirectory(getLocalEncodingFileName(QFileInfo(fileName).canonicalPath()));
	auto const & seriesUID = nameGenerator->GetSeriesUIDs();
	std::string seriesIdentifier = seriesUID.begin()->c_str();
	auto fileNames = nameGenerator->GetFileNames(seriesIdentifier);
	reader->SetFileNames(fileNames);
	reader->Update();
	reader->Modified();
	reader->Update();

	return std::make_shared<iAImageData>(reader->GetOutput());
}

template <typename T>
void writeDCM_template(QString const& fileName, iAProgress const & p, iAITKIO::ImagePtr itkImgBase, bool comp)
{
	// TODO: some overlap to iAImageStackFileIO!
	typedef itk::Image<T, iAITKIO::Dim> InputImageType;
	typedef itk::Image<T, iAITKIO::Dim - 1> OutputImageType;
	typedef itk::ImageSeriesWriter<InputImageType, OutputImageType> SeriesWriterType;
	auto writer = SeriesWriterType::New();

	typedef itk::NumericSeriesFileNames NameGeneratorType;
	auto nameGenerator = NameGeneratorType::New();

	auto itkImg = dynamic_cast<InputImageType*>(itkImgBase);
	typename InputImageType::RegionType region = itkImg->GetLargestPossibleRegion();
	typename InputImageType::IndexType start = region.GetIndex();
	typename InputImageType::SizeType size = region.GetSize();
	nameGenerator->SetStartIndex(start[2]);
	nameGenerator->SetEndIndex(start[2] + size[2] - 1);
	nameGenerator->SetIncrementIndex(1);

	QFileInfo fi(fileName);

	auto gdcmIO = itk::GDCMImageIO::New();
	itk::MetaDataDictionary& dict = gdcmIO->GetMetaDataDictionary();
	std::string tagkey, value;
	tagkey = "0008|0060";  //Modality
	value =
		"CT";  //Computed Tomography (https://wiki.nci.nih.gov/display/CIP/Key+to+two-letter+Modality+Acronyms+in+DICOM)
	itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
	tagkey = "0008|0008";  //Image Type
	value = "ORIGINAL";    //Original image
	itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
	tagkey = "0008|0064";  //Conversion Type
	value = "SI";          //Scanned Image
	itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
	writer->SetImageIO(gdcmIO);

	QString length = QString::number(size[2]);
	QString format(
		fi.absolutePath() + "/" + fi.baseName() + "%0" + QString::number(length.size()) + "d." + fi.completeSuffix());
	nameGenerator->SetSeriesFormat(getLocalEncodingFileName(format).c_str());
	writer->SetFileNames(nameGenerator->GetFileNames());
	writer->SetInput(itkImg);
	writer->SetUseCompression(comp);
	p.observe(writer);
	writer->Update();
}

void iADCMFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet,
	QVariantMap const& paramValues, iAProgress const& progress)
{
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	auto itkImgBase = imgData->itkImage();
	auto scalarType = ::itkScalarType(itkImgBase);
	ITK_TYPED_CALL(writeDCM_template, scalarType, fileName, progress, itkImgBase,
		paramValues[iAFileIO::CompressionStr].toBool());
}

QString iADCMFileIO::name() const
{
	return Name;
}

QStringList iADCMFileIO::extensions() const
{
	return QStringList{ "dcm" };
}
