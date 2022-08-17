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


// tested both loading files via ITK and VTK; ITK method is faster (see comments below)
#define RAW_LOAD_METHOD ITK

#if RAW_LOAD_METHOD == ITK

	#include "iAConnector.h"
	#include "iARawFileParameters.h"
	#include "iATypedCallHelper.h"

	#include <itkImage.h>
	#include <itkRawImageIO.h>
	#include <itkImageFileReader.h>

	#include <vtkImageData.h>

#else // VTK

	#include <vtkImageReader2.h>

#endif

#include <QElapsedTimer>
#include <QFileInfo>


iARawFileIO::iARawFileIO() : iAFileIO(iADataSetType::Volume)
{
	auto datatype = readableDataTypeList(false);
	QString selectedType = mapVTKTypeToReadableDataType(VTK_UNSIGNED_SHORT);
	selectOption(datatype, selectedType);
	auto byteOrders = readableByteOrderList();
	auto defaultByteOrder = "Little Endian";
	selectOption(byteOrders, defaultByteOrder);
	addParameter("Size", iAValueType::Vector3i, QVariant::fromValue(QVector<int>{1, 1, 1}));
	addParameter("Spacing", iAValueType::Vector3, QVariant::fromValue(QVector<double>{1.0, 1.0, 1.0}));
	addParameter("Origin", iAValueType::Vector3, QVariant::fromValue(QVector<double>{0.0, 0.0, 0.0}));
	addParameter("Headersize", iAValueType::Discrete, 0, 0);
	addParameter("Data Type", iAValueType::Categorical, datatype);
	addParameter("Byte Order", iAValueType::Categorical, byteOrders);
}

#if RAW_LOAD_METHOD == ITK
template <class T>
void read_raw_image_template(iARawFileParameters const& params, QString const& fileName, iAProgress* progress, iAConnector& image)
{
	typedef itk::RawImageIO<T, DIM> RawImageIOType;
	auto io = RawImageIOType::New();
	io->SetFileName(getLocalEncodingFileName(fileName).c_str());
	io->SetHeaderSize(params.m_headersize);
	for (int i = 0; i < DIM; i++)
	{
		io->SetDimensions(i, params.m_size[i]);
		io->SetSpacing(i, params.m_spacing[i]);
		io->SetOrigin(i, params.m_origin[i]);
	}
	if (params.m_byteOrder == VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN)
	{
		io->SetByteOrderToLittleEndian();
	}
	else
	{
		io->SetByteOrderToBigEndian();
	}

	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	auto reader = ReaderType::New();
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

std::shared_ptr<iADataSet> iARawFileIO::load(iAProgress* p, QMap<QString, QVariant> const& parameters)
{
	Q_UNUSED(parameters);
	QElapsedTimer t;
	t.start();

	// ITK way:
	iAConnector con;
	iARawFileParameters rawFileParams;
	auto sizeVec = parameters["Size"].value<QVector<int>>();
	auto spcVec = parameters["Spacing"].value<QVector<double>>();
	auto oriVec = parameters["Origin"].value<QVector<double>>();
	for (int c = 0; c < 3; ++c)
	{
		rawFileParams.m_size[c] = sizeVec[c];
		rawFileParams.m_spacing[c] = spcVec[c];
		rawFileParams.m_origin[c] = oriVec[c];
	}
	rawFileParams.m_headersize = parameters["Headersize"].toULongLong();
	rawFileParams.m_scalarType = mapReadableDataTypeToVTKType(parameters["Data Type"].toString());
	rawFileParams.m_byteOrder = mapReadableByteOrderToVTKType(parameters["Byte Order"].toString());

#if RAW_LOAD_METHOD == ITK
	VTK_TYPED_CALL(read_raw_image_template, rawFileParams.m_scalarType, rawFileParams, m_fileName, p, con);
	// direct copying as in following line would cause a crash further down the line:
	// auto img = con.vtkImage();
	// instead, we need to do a deep copy here:
	auto img = vtkSmartPointer<vtkImageData>::New();
	img->DeepCopy(con.vtkImage());
#else // VTK
	vtkSmartPointer<vtkImageReader2> reader = vtkSmartPointer<vtkImageReader2>::New();
	reader->SetFileName(m_fileName.toStdString().c_str());
	auto size = parameters["Size"].value<QVector<int>>();
	auto spacing = parameters["Spacing"].value<QVector<double>>();
	auto origin = parameters["Origin"].value<QVector<double>>();
	reader->SetDataExtent(0, size[0] - 1, 0, size[1] - 1, 0, size[2] - 1);
	reader->SetDataSpacing(spacing[0], spacing[1], spacing[2]);
	reader->SetDataOrigin(origin[0], origin[1], origin[2]);
	reader->SetHeaderSize(parameters["Headersize"].toUInt());
	reader->SetDataScalarType(mapReadableDataTypeToVTKType(parameters["Data Type"].toString()));
	reader->SetDataByteOrder(mapReadableByteOrderToVTKType(parameters["Byte Order"].toString()));
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

	auto ret = std::make_shared<iAImageData>(QFileInfo(m_fileName).baseName(), m_fileName, img);
	LOG(lvlInfo, QString("Elapsed: %1 ms.").arg(t.elapsed()));
	// TODO: maybe compute range here as well?
	//auto rng = img->GetScalarRange();
	//LOG(lvlInfo, QString("After min/max: %1 ms.").arg(t.elapsed()));
	return ret;
}

QString iARawFileIO::Name("RAW files");

QString iARawFileIO::name() const
{
	return Name;
}

QStringList iARawFileIO::extensions() const
{
	return QStringList{ "raw", "vol", "rec", "pro" };
}