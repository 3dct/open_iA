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
#include "iARawFileIO.h"

#include "defines.h"       // for DIM
#include "iAFileUtils.h"   // for getLocalEncodingFileName
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
	auto byteOrders = ByteOrder::stringList();
	selectOption(byteOrders, ByteOrder::LittleEndianStr);
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
void readRawImage(QVariantMap const& params, QString const& fileName, iAConnector& image, iAProgress* progress)
{
	auto io = itk::RawImageIO<T, DIM>::New();
	io->SetFileName(getLocalEncodingFileName(fileName).c_str());
	io->SetHeaderSize(params[iARawFileIO::HeadersizeStr].toInt());
	for (int i = 0; i < DIM; i++)
	{
		io->SetDimensions(i, params[iARawFileIO::SizeStr].value<QVector<int>>()[i]);
		io->SetSpacing(i, params[iARawFileIO::SpacingStr].value<QVector<double>>()[i]);
		io->SetOrigin(i, params[iARawFileIO::OriginStr].value<QVector<double>>()[i]);
	}
	if (params[iARawFileIO::ByteOrderStr].toString() == ByteOrder::LittleEndianStr)
	{
		io->SetByteOrderToLittleEndian();
	}
	else
	{
		io->SetByteOrderToBigEndian();
	}
	auto reader = itk::ImageFileReader<itk::Image<T, DIM>>::New();
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	reader->SetImageIO(io);
	progress->observe(reader);
	reader->Modified();
	reader->ReleaseDataFlagOn();
	reader->Update();
	image.setImage(reader->GetOutput());
	image.modified();
}
#endif

std::vector<std::shared_ptr<iADataSet>> iARawFileIO::load(QString const& fileName, QVariantMap const& parameters, iAProgress* progress)
{
	// ITK way:
	iAConnector con;

#if RAW_LOAD_METHOD == ITK
	auto scalarType = mapReadableDataTypeToVTKType(parameters[DataTypeStr].toString());
	VTK_TYPED_CALL(readRawImage, scalarType, parameters, fileName, con, progress);
	// direct copying as in following line would cause a crash further down the line:
	// auto img = con.vtkImage();
	// instead, we need to do a deep copy here:
	auto img = vtkSmartPointer<vtkImageData>::New();
	img->DeepCopy(con.vtkImage());
#else // VTK
	vtkSmartPointer<vtkImageReader2> reader = vtkSmartPointer<vtkImageReader2>::New();
	reader->SetFileName(m_fileName.toStdString().c_str());
	auto size = parameters[SizeStr].value<QVector<int>>();
	auto spacing = parameters[SpacingStr].value<QVector<double>>();
	auto origin = parameters[OriginStr].value<QVector<double>>();
	reader->SetDataExtent(0, size[0] - 1, 0, size[1] - 1, 0, size[2] - 1);
	reader->SetDataSpacing(spacing[0], spacing[1], spacing[2]);
	reader->SetDataOrigin(origin[0], origin[1], origin[2]);
	reader->SetHeaderSize(parameters[HeadersizeStr].toUInt());
	reader->SetDataScalarType(mapReadableDataTypeToVTKType(parameters[DataTypeStr].toString()));
	reader->SetDataByteOrder(mapReadableByteOrderToVTKType(parameters[ByteOrderStr].toString()));
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
	return { std::make_shared<iAImageData>(fileName, img) };
	// TODO: maybe compute range here as well?
	//auto rng = img->GetScalarRange();   // see also comments above about performance measurements
}

template<class T>
void writeRawImage(QString const& fileName, vtkImageData* img, QVariantMap parameters, iAProgress* progress)
{
	iAConnector con;
	con.setImage(img);
	using InputImageType = itk::Image<T, DIM>;
	auto writer = itk::ImageFileWriter<InputImageType>::New();
	auto io = itk::RawImageIO<T, DIM>::New();
	io->SetFileName(getLocalEncodingFileName(fileName).c_str());
	//io->SetHeaderSize(0);
	//for (int i = 0; i < DIM; i++)
	//{
	//	io->SetDimensions(i, img->GetDimensions()[i]);
	//	io->SetSpacing(i, img->GetSpacing()[i]);
	//	io->SetOrigin(i, img->GetOrigin()[i]);
	//}
	if (parameters[iARawFileIO::ByteOrderStr].toString() == ByteOrder::LittleEndianStr)
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
	writer->SetUseCompression(parameters[iAFileIO::CompressionStr].toBool());
	progress->observe(writer);
	writer->Update();
}

void iARawFileIO::save(QString const& fileName, std::vector<std::shared_ptr<iADataSet>> const& dataSets, QVariantMap const& parameters, iAProgress* progress)
{
	// ITK way:
	assert(dataSets.size() == 1 && dynamic_cast<iAImageData*>(dataSets[0].get()));
//#if RAW_LOAD_METHOD == ITK
	auto scalarType = mapReadableDataTypeToVTKType(parameters[DataTypeStr].toString());
	VTK_TYPED_CALL(writeRawImage, scalarType, fileName, dynamic_cast<iAImageData*>(dataSets[0].get())->image(), parameters, progress);
//# endif
// VTK way currently not implemented
}

QString iARawFileIO::name() const
{
	return Name;
}

QStringList iARawFileIO::extensions() const
{
	return QStringList{ "raw", "vol", "rec", "pro" };
}