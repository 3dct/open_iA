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

std::shared_ptr<iAFileIO> iAImageStackFileIO::create()
{
	return std::make_shared<iAImageStackFileIO>();
}

std::vector<std::shared_ptr<iADataSet>> iAImageStackFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress* progress)
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
	progress->observe(imgReader);
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
	return { std::make_shared<iAImageData>(fileName, img) };
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

#include "iAConnector.h"
#include "iAExtendedTypedCallHelper.h"

#include <itkBMPImageIO.h>
#include <itkJPEGImageIO.h>
#include <itkPNGImageIO.h>
#include <itkTIFFImageIO.h>
#include <itkImage.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>

template <typename T>
void writeImageStack(QString const& fileName, iAConnector const & con, bool comp, iAProgress* p)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM - 1>;
	auto writer = itk::ImageSeriesWriter<InputImageType, OutputImageType>::New();
	auto itkImg = dynamic_cast<InputImageType*>(con.itkImage());
	auto region = itkImg->GetLargestPossibleRegion();
	auto start = region.GetIndex();
	auto size = region.GetSize();
	QFileInfo fi(fileName);
	// set IO explicitly, to avoid SCIFIO claiming being able to write those image formats and then failing:
	auto ext = fi.completeSuffix().toLower();
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
	writer->SetImageIO(imgIO);
	QString length = QString::number(size[2]);
	QString format(fi.absolutePath() + "/" + fi.baseName() + "%0" + QString::number(length.size()) + "d." + fi.completeSuffix());
	auto nameGenerator = itk::NumericSeriesFileNames::New();
	nameGenerator->SetStartIndex(start[2]);
	nameGenerator->SetEndIndex(start[2] + size[2] - 1);
	nameGenerator->SetIncrementIndex(1);
	nameGenerator->SetSeriesFormat(getLocalEncodingFileName(format).c_str());
	writer->SetFileNames(nameGenerator->GetFileNames());
	writer->SetInput(itkImg);
	writer->SetUseCompression(comp);
	p->observe(writer);
	writer->Update();
}

void iAImageStackFileIO::save(QString const& fileName, std::vector<std::shared_ptr<iADataSet>> const& dataSets, QVariantMap const& paramValues, iAProgress* progress)
{
	Q_UNUSED(paramValues);
	assert(dataSets.size() == 1);
	auto img = dynamic_cast<iAImageData*>(dataSets[0].get())->vtkImage();
	iAConnector con;
	con.setImage(img);
	auto pixelType = con.itkScalarPixelType();
	auto imagePixelType = con.itkPixelType();
	ITK_EXTENDED_TYPED_CALL(writeImageStack, pixelType, imagePixelType,
		fileName, con, paramValues[iAFileIO::CompressionStr].toBool(), progress);
}

bool iAImageStackFileIO::s_bRegistered = iAFileTypeRegistry::addFileType(iAImageStackFileIO::create);
