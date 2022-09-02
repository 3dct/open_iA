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
#include "iAImageStackFileIO.h"

#include "defines.h"         // for DIM
#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileUtils.h"     // for getLocalEncodingFileName
#include "iAProgress.h"
#include "iAStringHelper.h"  // for greatestCommonPrefix
#include "iAToolsVTK.h"      // for mapVTKTypeToReadableDataType, readableDataTypes, ...
#include "iAValueTypeVectorHelpers.h"

#include <QDir>
#include <QFileInfo>


//#define VTK 0
//#define ITK 1
//#define STACK_LOAD_METHOD VTK

//#if STACK_LOAD_METHOD == ITK
//#else // VTK

	#include <vtkCommand.h>
	#include <vtkImageData.h>
	#include <vtkImageReader2.h>

	#include <vtkBMPReader.h>
	#include <vtkJPEGReader.h>
	#include <vtkPNGReader.h>
	#include <vtkTIFFReader.h>

	#include <vtkStringArray.h>

//#endif

QString const iAImageStackFileIO::LoadTypeStr("Loading Type");
QString const iAImageStackFileIO::SingleImageOption("Single Image");
QString const iAImageStackFileIO::ImageStackOption("Image Stack");
QString const iAImageStackFileIO::Name("Image (Stack)");
QString const iAImageStackFileIO::FileNameBase("File name base");
QString const iAImageStackFileIO::Extension("Extension");
QString const iAImageStackFileIO::NumDigits("Number of digits in index");
QString const iAImageStackFileIO::MinimumIndex("Minimum index");
QString const iAImageStackFileIO::MaximumIndex("Maximum index");

namespace
{
	QString const SpacingStr("Spacing");
	QString const StepStr("Step");
	QString const OriginStr("Origin");

	vtkNew<vtkStringArray> fileNameArray(QString fileNameBase, QString const & suffix, int* indexRange, int digitsInIndex, int stepSize)
	{
		vtkNew<vtkStringArray> result;
		for (int i = indexRange[0]; i <= indexRange[1]; i += stepSize)
		{
			QString temp = fileNameBase + QString("%1").arg(i, digitsInIndex, 10, QChar('0')) + suffix;
			result->InsertNextValue(getLocalEncodingFileName(temp).c_str());
		}
		return result;
	}
}

iAImageStackFileIO::iAImageStackFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{
	QStringList loadTypes = QStringList() << SingleImageOption << ImageStackOption;
	addAttr(m_params[Load], LoadTypeStr, iAValueType::Categorical, loadTypes);
	addAttr(m_params[Load], StepStr, iAValueType::Discrete, 1);
	addAttr(m_params[Load], SpacingStr, iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
	addAttr(m_params[Load], OriginStr, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addAttr(m_params[Load], FileNameBase, iAValueType::String, "");
	addAttr(m_params[Load], Extension, iAValueType::String, "");
	addAttr(m_params[Load], NumDigits, iAValueType::Discrete, 0);
	addAttr(m_params[Load], MinimumIndex, iAValueType::Discrete, 0);
	addAttr(m_params[Load], MaximumIndex, iAValueType::Discrete, 0);

	addAttr(m_params[Save], CompressionStr, iAValueType::Boolean, false);
}

std::vector<std::shared_ptr<iADataSet>> iAImageStackFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
//#if RAW_LOAD_METHOD == ITK
//#else
	auto ext = QFileInfo(fileName).suffix().toLower();

	vtkSmartPointer<vtkImageReader2> imgReader;
	if (ext.endsWith("jpg") || ext.endsWith("jpeg"))
	{
		imgReader = vtkSmartPointer<vtkJPEGReader>::New();
	}
	else if (ext.endsWith("tif") || ext.endsWith("tiff"))
	{
		imgReader = vtkSmartPointer<vtkTIFFReader>::New();
	}
	else if (ext.endsWith("png"))
	{
		imgReader = vtkSmartPointer<vtkPNGReader>::New();
	}
	else if (ext.endsWith("bmp"))
	{
		imgReader = vtkSmartPointer<vtkBMPReader>::New();
	}
	else
	{
		throw std::runtime_error(QString("Unknown image extension '%1'!").arg(ext).toStdString());
	}
	imgReader->ReleaseDataFlagOn();
	progress.observe(imgReader);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());

	if (paramValues[LoadTypeStr] == SingleImageOption)
	{
		imgReader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	}
	else
	{
		int indexRange[2];
		int digits;
		auto fileNameBase = paramValues[FileNameBase].toString();
		auto suffix = paramValues[Extension].toString();
		digits = paramValues[NumDigits].toInt();
		indexRange[0] = paramValues[MinimumIndex].toInt();
		indexRange[1] = paramValues[MaximumIndex].toInt();
		int stepSize = paramValues[StepStr].toInt();
		double spacing[3], origin[3];
		setFromVectorVariant<double>(spacing, paramValues[SpacingStr]);
		setFromVectorVariant<double>(origin, paramValues[OriginStr]);
		auto fileNames = fileNameArray(fileNameBase, suffix, indexRange, digits, stepSize);
		imgReader->SetFileNames(fileNames);
		imgReader->SetDataOrigin(origin);
		imgReader->SetDataSpacing(spacing);
	}
	auto img = vtkSmartPointer<vtkImageData>::New();
	imgReader->SetOutput(img);
	imgReader->Update();
	return { std::make_shared<iAImageData>(img) };
	// TODO: maybe compute range here as well?
	//auto rng = img->GetScalarRange();   // see also comments above about performance measurements
//#endif
}

QString iAImageStackFileIO::name() const
{
	return Name;
}

QStringList iAImageStackFileIO::extensions() const
{
	return QStringList{ "bmp", "jpg", "jpeg", "png", "tif", "tiff" };
}

bool iAImageStackFileIO::isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const
{
	auto ext = QFileInfo(fileName).suffix().toLower();
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	assert(imgData);
	auto type = imgData->vtkImage()->GetScalarType();
	return type == VTK_UNSIGNED_CHAR || // supported by all file formats
		((ext == "tif" || ext == "tiff") && (type == VTK_UNSIGNED_SHORT || type == VTK_FLOAT));
}

#include "iAExtendedTypedCallHelper.h"
#include "iAToolsITK.h"

#include <itkBMPImageIO.h>
#include <itkJPEGImageIO.h>
#include <itkPNGImageIO.h>
#include <itkTIFFImageIO.h>
#include <itkImage.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>

template <typename T>
void writeImageStack(itk::ImageBase<3>* img,
	QString const & base, QString const & suffix, int numDigits, int minIdx, int maxIdx, bool comp,
	iAProgress const& progress)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM - 1>;
	auto writer = itk::ImageSeriesWriter<InputImageType, OutputImageType>::New();
	// set IO explicitly, to avoid SCIFIO claiming being able to write those image formats and then failing:
	auto ext = suffix.toLower();
	itk::ImageIOBase::Pointer imgIO;
	if (ext == "bmp")
	{
		imgIO = itk::BMPImageIO::New();
	}
	else if (ext == "jpg" || ext == "jpeg")
	{
		imgIO = itk::JPEGImageIO::New();
	}
	else if (ext == "png")
	{
		imgIO = itk::PNGImageIO::New();
	}
	else if (ext == "tif" || ext == "tiff")
	{
		imgIO = itk::TIFFImageIO::New();
	}
	else
	{
		throw std::runtime_error(QString("Unknown IO extension %1 in image stack writer!").arg(ext).toStdString());
	}
	writer->SetImageIO(imgIO);
	QString format(base + "%0" + QString::number(numDigits) + "d." + suffix);
	auto nameGenerator = itk::NumericSeriesFileNames::New();
	nameGenerator->SetStartIndex(minIdx);
	nameGenerator->SetEndIndex(maxIdx);
	nameGenerator->SetIncrementIndex(1);
	nameGenerator->SetSeriesFormat(getLocalEncodingFileName(format).c_str());
	writer->SetFileNames(nameGenerator->GetFileNames());
	writer->SetInput(dynamic_cast<InputImageType*>(img));
	writer->SetUseCompression(comp);
	progress.observe(writer);
	writer->Update();
}

void iAImageStackFileIO::saveData(QString const& fileName, std::vector<std::shared_ptr<iADataSet>>& dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	assert(dataSets.size() == 1);

	QFileInfo fi(fileName);
	QString base = fi.absolutePath() + "/" + fi.baseName();
	QString suffix = fi.completeSuffix();
	auto imgData = dynamic_cast<iAImageData*>(dataSets[0].get());
	auto itkImg = imgData->itkImage();
	auto region = itkImg->GetLargestPossibleRegion();
	auto start = region.GetIndex();
	auto size = region.GetSize();
	int numDigits = QString::number(size[2]).size();  // number of digits in z size number string
	int minIdx = start[2];
	int maxIdx = start[2] + size[2] - 1;
	auto pixelType = ::itkScalarPixelType(itkImg);
	auto imagePixelType = ::itkPixelType(itkImg);
	ITK_EXTENDED_TYPED_CALL(writeImageStack, pixelType, imagePixelType,
		itkImg, base, suffix, numDigits, minIdx, maxIdx,
		paramValues[iAFileIO::CompressionStr].toBool(), progress);

	dataSets[0]->setMetaData(LoadTypeStr, ImageStackOption);
	dataSets[0]->setMetaData(StepStr, 1);
	dataSets[0]->setMetaData(SpacingStr, variantVector(imgData->vtkImage()->GetSpacing(), 3));
	dataSets[0]->setMetaData(OriginStr, variantVector(imgData->vtkImage()->GetOrigin(), 3));
	dataSets[0]->setMetaData(FileNameBase, base);
	dataSets[0]->setMetaData(Extension, suffix);
	dataSets[0]->setMetaData(NumDigits, numDigits);
	dataSets[0]->setMetaData(MinimumIndex, minIdx);
	dataSets[0]->setMetaData(MaximumIndex, maxIdx);
}
