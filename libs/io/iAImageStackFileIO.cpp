// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageStackFileIO.h"

#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileStackParams.h"
#include "iAFileUtils.h"     // for getLocalEncodingFileName
#include "iAImageData.h"
#include "iAMathUtility.h"   // for clamp
#include "iAProgress.h"
#include "iAStringHelper.h"  // for greatestCommonPrefix
#include "iAToolsVTK.h"      // for mapVTKTypeToReadableDataType, readableDataTypes, ...
#include "iATypedCallHelper.h"
#include "iAValueTypeVectorHelpers.h"

#include "iAITKFileIO.h"

#include <QDir>
#include <QFileInfo>

#include <vtkImageReslice.h>


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
QString const iAImageStackFileIO::AxisOption("Axis");

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
	QStringList axesOptions = QStringList() << "X" << "Y" << "Z";

	QStringList loadTypes = QStringList() << SingleImageOption << ImageStackOption;
	addAttr(m_params[Load], LoadTypeStr, iAValueType::Categorical, loadTypes);
	addAttr(m_params[Load], StepStr, iAValueType::Discrete, 1);
	addAttr(m_params[Load], SpacingStr, iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
	addAttr(m_params[Load], OriginStr, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addAttr(m_params[Load], iAFileStackParams::FileNameBase, iAValueType::String, "");
	addAttr(m_params[Load], iAFileStackParams::Extension, iAValueType::String, "");
	addAttr(m_params[Load], iAFileStackParams::NumDigits, iAValueType::Discrete, 0);
	addAttr(m_params[Load], iAFileStackParams::MinimumIndex, iAValueType::Discrete, 0);
	addAttr(m_params[Load], iAFileStackParams::MaximumIndex, iAValueType::Discrete, 0);
	// TODO: Maybe support in a future version
	//addAttr(m_params[Load], AxisOption, iAValueType::Categorical, axesOptions);
	// no file name parameter in image stack:
	auto it = std::find_if(m_params[Load].begin(), m_params[Load].end(), [](auto a) { return a->name() == iADataSet::FileNameKey; });
	if (it != m_params[Load].end())
	{
		m_params[Load].erase(it);
	}

	addAttr(m_params[Save], CompressionStr, iAValueType::Boolean, false);
	addAttr(m_params[Save], AxisOption, iAValueType::Categorical, axesOptions);
	addAttr(m_params[Save], iAFileStackParams::MinimumIndex, iAValueType::Discrete, 0);
	addAttr(m_params[Save], iAFileStackParams::MaximumIndex, iAValueType::Discrete, 0);
}

std::shared_ptr<iADataSet> iAImageStackFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	if (paramValues[LoadTypeStr] == SingleImageOption)
	{
		iAITKFileIO io;
		auto ds = io.loadData(fileName, QVariantMap(), progress);
		ds->setMetaData(LoadTypeStr, SingleImageOption);
		return ds;
	}
//#if RAW_LOAD_METHOD == ITK
// 	   test itkImageSeriesReader ?
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

	int indexRange[2];
	int digits;
	auto fileNameBase = paramValues[iAFileStackParams::FileNameBase].toString();
	auto suffix = paramValues[iAFileStackParams::Extension].toString();
	digits = paramValues[iAFileStackParams::NumDigits].toInt();
	indexRange[0] = paramValues[iAFileStackParams::MinimumIndex].toInt();
	indexRange[1] = paramValues[iAFileStackParams::MaximumIndex].toInt();
	int stepSize = paramValues[StepStr].toInt();
	double spacing[3], origin[3];
	setFromVectorVariant<double>(spacing, paramValues[SpacingStr]);
	setFromVectorVariant<double>(origin, paramValues[OriginStr]);
	auto fileNames = fileNameArray(fileNameBase, suffix, indexRange, digits, stepSize);
	imgReader->SetFileNames(fileNames);
	imgReader->SetDataOrigin(origin);
	imgReader->SetDataSpacing(spacing);
	auto img = vtkSmartPointer<vtkImageData>::New();
	imgReader->SetOutput(img);
	imgReader->Update();
	auto ds = std::make_shared<iAImageData>(img);
	ds->setMetaData(paramValues);
	return ds;
}

QString iAImageStackFileIO::name() const
{
	return Name;
}

QStringList iAImageStackFileIO::extensions() const
{
	return QStringList{ "bmp", "jpg", "jpeg", "png", "tif", "tiff" }; //TODO NEWIO: check jpeg2000 support
}

bool iAImageStackFileIO::isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName, Operation op) const
{
	auto ext = QFileInfo(fileName).suffix().toLower();
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	assert(imgData);
	auto type = imgData->vtkImage()->GetScalarType();
	auto result = iAFileIO::isDataSetSupported(dataSet, fileName, op) && (
			type == VTK_UNSIGNED_CHAR || // supported by all file formats
			((ext == "tif" || ext == "tiff") && (type == VTK_UNSIGNED_SHORT || type == VTK_FLOAT))
		);
	if (!result)
	{
		LOG(lvlInfo, QString("Image stack only supports unsigned char (8 bit) data type (for .tif stacks, also unsigned short (16 bit) and float (32 bit)). You need to manually convert the dataset first!"));
	}
	return result;
}

//#include "iASlicerMode.h"    // for iAAxisIndex -> is in iAguibase!
enum iAAxisIndex {    // TODO: merge with iASlicerMode.h iAAxisIndex
	X, Y, Z
};

void writeImageStack(vtkSmartPointer<vtkImageData> img,
	QString const& base, QString const& suffix, int numDigits, int minIdx, int maxIdx, bool comp,
	int axisIdx, iAProgress const& progress)
{
	vtkNew<vtkImageReslice> reslicer;
	reslicer->SetInputData(img);
	reslicer->SetOutputDimensionality(2);
	reslicer->InterpolateOff();
	reslicer->SetEnableSMP(true);
	switch (axisIdx)
	{
		case iAAxisIndex::X: reslicer->SetResliceAxesDirectionCosines(0, 1, 0, 0, 0, 1, 1,  0, 0); break;
		case iAAxisIndex::Y: reslicer->SetResliceAxesDirectionCosines(1, 0, 0, 0, 0, 1, 0, -1, 0); break;
		case iAAxisIndex::Z: reslicer->SetResliceAxesDirectionCosines(1, 0, 0, 0, 1, 0, 0,  0, 1); break;
	}
	auto ext = img->GetExtent();
	auto spc = img->GetSpacing();
	auto ori = img->GetOrigin();
	double xyz[3] = { 0.0, 0.0, 0.0 };
	minIdx = clamp(ext[axisIdx * 2], ext[axisIdx * 2 + 1], minIdx);
	maxIdx = clamp(ext[axisIdx * 2], ext[axisIdx * 2 + 1], maxIdx);
	for (int i = minIdx; i <= maxIdx; ++i)
	{
		xyz[axisIdx] = i;
		reslicer->SetResliceAxesOrigin(ori[0] + xyz[0] * spc[0], ori[1] + xyz[1] * spc[1], ori[2] + xyz[2] * spc[2]);
		reslicer->Update();
		auto sliceImg = reslicer->GetOutput();
		QString fileName = base + QString("%1").arg(i, numDigits, 10, QChar('0')) + "." + suffix;
		storeImage(sliceImg, fileName, comp);
		progress.emitProgress(100.0 * static_cast<double>(i - minIdx) / (maxIdx - minIdx));
	}
}

// "legacy" code for writing images stacks; kept because it writes the stack in z-direction much faster than above code
// (~500ms vs ~5500ms for 400x400x400 dataset):

#include "iAConnector.h"
#include "iAToolsITK.h"

#include <itkBMPImageIO.h>
#include <itkJPEGImageIO.h>
#include <itkPNGImageIO.h>
#include <itkTIFFImageIO.h>
#include <itkImage.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>

template <typename T>
void simpleWriteImageStack(itk::ImageBase<3>* img,
	QString const& base, QString const& suffix, int numDigits, int minIdx, int maxIdx, bool comp,
	iAProgress const& progress)
{
	using InputImageType = itk::Image<T, iAITKIO::Dim>;
	using OutputImageType = itk::Image<T, iAITKIO::Dim - 1>;
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

int iAImageStackFileIO::axisName2Idx(QString const & axisName)
{
	if (axisName == "X") { return iAAxisIndex::X; }
	else if (axisName == "Y") { return iAAxisIndex::Y; }
	else return iAAxisIndex::Z;
}

void iAImageStackFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);

	QFileInfo fi(fileName);
	QString base = fi.absolutePath() + "/" + fi.baseName();
	QString suffix = fi.completeSuffix();
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	auto vtkImg = imgData->vtkImage();
	int minIdx = paramValues[iAFileStackParams::MinimumIndex].toInt(),
		maxIdx = paramValues[iAFileStackParams::MaximumIndex].toInt();
	int axisIdx = axisName2Idx(paramValues[AxisOption].toString());
	auto ext = vtkImg->GetExtent();
	int numDigits = static_cast<int>(QString::number(ext[axisIdx * 2 + 1]).size());  // number of digits in z size number string
	if (axisIdx == iAAxisIndex::Z)
	{
		iAConnector con;
		con.setImage(vtkImg);
		ITK_TYPED_CALL(simpleWriteImageStack, con.itkScalarType(), con.itkImage(), base, suffix, numDigits, minIdx, maxIdx, paramValues[iAFileIO::CompressionStr].toBool(), progress);
	}
	else
	{
		writeImageStack(vtkImg, base, suffix, numDigits, minIdx, maxIdx, paramValues[iAFileIO::CompressionStr].toBool(), axisIdx, progress);
	}

	dataSet->setMetaData(LoadTypeStr, ImageStackOption);
	dataSet->setMetaData(StepStr, 1);
	dataSet->setMetaData(SpacingStr, variantVector(imgData->vtkImage()->GetSpacing(), 3));
	dataSet->setMetaData(OriginStr, variantVector(imgData->vtkImage()->GetOrigin(), 3));
	dataSet->setMetaData(iAFileStackParams::FileNameBase, base);
	dataSet->setMetaData(iAFileStackParams::Extension, suffix);
	dataSet->setMetaData(iAFileStackParams::NumDigits, numDigits);
	dataSet->setMetaData(iAFileStackParams::MinimumIndex, minIdx);
	dataSet->setMetaData(iAFileStackParams::MaximumIndex, maxIdx);
}
