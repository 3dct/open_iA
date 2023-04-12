// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARawFileIO.h"

#include "iAFileUtils.h"   // for getLocalEncodingFileName
#include "iAITKIO.h"       // for iAITKIO::Dim
#include "iAProgress.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...
#include "iAValueTypeVectorHelpers.h"


// tested both loading files via ITK and VTK; ITK method is faster (see comments below)
#define RAW_LOAD_METHOD ITK

#if RAW_LOAD_METHOD == ITK

	#include "iAConnector.h"
	#include "iATypedCallHelper.h"

	#include <itkImage.h>
	#include <itkRawImageIO.h>
	#include <itkImageFileReader.h>
	#include <itkImageFileWriter.h>

	#include <vtkImageData.h>

#else // VTK

	#include <vtkImageReader2.h>

#endif

const QString iARawFileIO::Name("RAW files");
const QString iARawFileIO::SizeStr("Size");
const QString iARawFileIO::SpacingStr("Spacing");
const QString iARawFileIO::OriginStr("Origin");
const QString iARawFileIO::HeadersizeStr("Headersize");
const QString iARawFileIO::DataTypeStr("Data Type");
const QString iARawFileIO::ByteOrderStr("Byte Order");

iARawFileIO::iARawFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{
	auto datatype = readableDataTypeList(false);
	QString selectedType = mapVTKTypeToReadableDataType(VTK_UNSIGNED_SHORT);
	selectOption(datatype, selectedType);
	auto byteOrders = iAByteOrder::stringList();
	selectOption(byteOrders, iAByteOrder::LittleEndianStr);
	addAttr(m_params[Load], SizeStr, iAValueType::Vector3i, variantVector<int>({1, 1, 1}));
	addAttr(m_params[Load], SpacingStr, iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
	addAttr(m_params[Load], OriginStr, iAValueType::Vector3, variantVector<double>({0.0, 0.0, 0.0}));
	addAttr(m_params[Load], HeadersizeStr, iAValueType::Discrete, 0, 0);
	addAttr(m_params[Load], DataTypeStr, iAValueType::Categorical, datatype);
	addAttr(m_params[Load], ByteOrderStr, iAValueType::Categorical, byteOrders);

	addAttr(m_params[Save], ByteOrderStr, iAValueType::Categorical, byteOrders);
}

#if RAW_LOAD_METHOD == ITK
template <class T>
void readRawImage(QVariantMap const& params, QString const& fileName, iAConnector& image, iAProgress const& progress)
{
	auto io = itk::RawImageIO<T, iAITKIO::Dim>::New();
	io->SetFileName(getLocalEncodingFileName(fileName).c_str());
	io->SetHeaderSize(params[iARawFileIO::HeadersizeStr].toInt());
	for (int i = 0; i < iAITKIO::Dim; i++)
	{
		io->SetDimensions(i, params[iARawFileIO::SizeStr].value<QVector<int>>()[i]);
		io->SetSpacing(i, params[iARawFileIO::SpacingStr].value<QVector<double>>()[i]);
		io->SetOrigin(i, params[iARawFileIO::OriginStr].value<QVector<double>>()[i]);
	}
	if (params[iARawFileIO::ByteOrderStr].toString() == iAByteOrder::LittleEndianStr)
	{
		io->SetByteOrderToLittleEndian();
	}
	else
	{
		io->SetByteOrderToBigEndian();
	}
	auto reader = itk::ImageFileReader<itk::Image<T, iAITKIO::Dim>>::New();
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	reader->SetImageIO(io);
	progress.observe(reader);
	reader->Modified();
	reader->ReleaseDataFlagOn();
	reader->Update();
	image.setImage(reader->GetOutput());
	image.modified();
}
#endif

std::shared_ptr<iADataSet> iARawFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	// ITK way:
	iAConnector con;

#if RAW_LOAD_METHOD == ITK
	auto scalarType = mapReadableDataTypeToVTKType(paramValues[DataTypeStr].toString());
	VTK_TYPED_CALL(readRawImage, scalarType, paramValues, fileName, con, progress);
	// direct copying as in following line would cause a crash further down the line:
	// auto img = con.vtkImage();
	// instead, we need to do a deep copy here:
	auto img = vtkSmartPointer<vtkImageData>::New();
	img->DeepCopy(con.vtkImage());
#else // VTK
	vtkSmartPointer<vtkImageReader2> reader = vtkSmartPointer<vtkImageReader2>::New();
	reader->SetFileName(m_fileName.toStdString().c_str());
	auto size = paramValues[SizeStr].value<QVector<int>>();
	auto spacing = paramValues[SpacingStr].value<QVector<double>>();
	auto origin = paramValues[OriginStr].value<QVector<double>>();
	reader->SetDataExtent(0, size[0] - 1, 0, size[1] - 1, 0, size[2] - 1);
	reader->SetDataSpacing(spacing[0], spacing[1], spacing[2]);
	reader->SetDataOrigin(origin[0], origin[1], origin[2]);
	reader->SetHeaderSize(paramValues[HeadersizeStr].toUInt());
	reader->SetDataScalarType(mapReadableDataTypeToVTKType(paramValues[DataTypeStr].toString()));
	reader->SetDataByteOrder(mapReadableByteOrderToVTKType(paramValues[ByteOrderStr].toString()));
	p->observe(reader);
	reader->UpdateWholeExtent();
	auto img = reader->GetOutput();
	// test dataset: 5.raw
	// on WELPC040, Release build with VS 2022 building VS2019 iAnalyze with Qt 5.15.2, VTK 9.0.3:
	// VTK: 1024, 1257, 1026 ms -> first suspected this might include computing min/max
	//      1011 (1038), 1014 (1034) times in parenthesis indicate time after accessing min/max (see GetScalarRange code below)
	// ITK: 173, 170, 163, 159, 160 ms
	//      165 (181), 165 (182), 162 (179) ms (including min/max computation)
	// Quick tests with CFK-Probe_Stahlstift_10xavg_freebeam_448proj.raw (~5GB):
	// VTK:  6690 (6956) ms
	// ITK: 51837 (52087) ms (erstes Lesen von Hard disk), 5463 (5721) ms (zweites lesen, nach VTK)
	//      -> ITK consistently faster, and much faster for small datasets!
#endif
	auto ds = std::make_shared<iAImageData>(img);
	ds->setMetaData(paramValues);
	return ds;
}

template<class T>
void writeRawImage(QString const& fileName, vtkImageData* img, QVariantMap paramValues, iAProgress const& progress)
{
	iAConnector con;
	con.setImage(img);
	using InputImageType = itk::Image<T, iAITKIO::Dim>;
	auto writer = itk::ImageFileWriter<InputImageType>::New();
	auto io = itk::RawImageIO<T, iAITKIO::Dim>::New();
	io->SetFileName(getLocalEncodingFileName(fileName).c_str());
	//io->SetHeaderSize(0);
	//for (int i = 0; i < DIM; i++)
	//{
	//	io->SetDimensions(i, img->GetDimensions()[i]);
	//	io->SetSpacing(i, img->GetSpacing()[i]);
	//	io->SetOrigin(i, img->GetOrigin()[i]);
	//}
	if (paramValues[iARawFileIO::ByteOrderStr].toString() == iAByteOrder::LittleEndianStr)
	{
		io->SetByteOrderToLittleEndian();
	}
	else
	{
		io->SetByteOrderToBigEndian();
	}
	writer->SetImageIO(io);
	writer->SetFileName(getLocalEncodingFileName(fileName).c_str());
	writer->SetInput(dynamic_cast<InputImageType*>(con.itkImage()));
	writer->SetUseCompression(paramValues[iAFileIO::CompressionStr].toBool());
	progress.observe(writer);
	writer->Update();
}

void iARawFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	// ITK way:
	assert(dynamic_cast<iAImageData*>(dataSet.get()));
//#if RAW_LOAD_METHOD == ITK
	auto vtkImg = dynamic_cast<iAImageData*>(dataSet.get())->vtkImage();
	VTK_TYPED_CALL(writeRawImage, vtkImg->GetScalarType(), fileName, vtkImg, paramValues, progress);
	// set raw file parameters here so that they are available if file gets stored in a project file!
	dataSet->setMetaData(SizeStr, variantVector(vtkImg->GetDimensions(), 3));
	dataSet->setMetaData(SpacingStr, variantVector(vtkImg->GetSpacing(), 3));
	dataSet->setMetaData(OriginStr, variantVector(vtkImg->GetOrigin(), 3));
	dataSet->setMetaData(HeadersizeStr, 0);
	dataSet->setMetaData(DataTypeStr, mapVTKTypeToReadableDataType(vtkImg->GetScalarType()));
//# endif
// VTK way currently not implemented
}

QString iARawFileIO::name() const
{
	return Name;
}

QStringList iARawFileIO::extensions() const
{
	return QStringList{ "raw", "vol", "rec", "pro", "pre" };
}
