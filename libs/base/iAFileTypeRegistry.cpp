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
#include "iAFileTypeRegistry.h"

#include <vtkPointData.h>
#include <vtkSTLReader.h>
#include <vtkUnsignedCharArray.h>

#include <QTextStream>

// ---------- iAFileIO ----------

iAFileIO::iAFileIO(iADataSetTypes types): m_dataSetTypes(types)
{}

void iAFileIO::setup(QString const & fileName)
{
	m_fileName = fileName;
}

iAFileIO::~iAFileIO()
{}

void iAFileIO::addParameter(QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
	m_parameters.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
}

iAAttributes const& iAFileIO::parameters() const
{
	return m_parameters;
}

iADataSetTypes iAFileIO::supportedDataSetTypes() const
{
	return m_dataSetTypes;
}

// ---------- iAFileTypeRegistry ----------

std::vector<std::shared_ptr<iAIFileIOFactory>> iAFileTypeRegistry::m_fileIOs;
QMap<QString, size_t> iAFileTypeRegistry::m_fileTypes;

std::shared_ptr<iAFileIO> iAFileTypeRegistry::createIO(QString const& fileExtension)
{
	if (m_fileTypes.contains(fileExtension))
	{
		return m_fileIOs[m_fileTypes[fileExtension]]->create();
	}
	else
	{
		return std::shared_ptr<iAFileIO>();
	}
}

QString iAFileTypeRegistry::registeredFileTypes(iADataSetTypes allowedTypes)
{
	QStringList allExtensions;
	QString singleTypes;
	for (auto ioFactory : m_fileIOs)  // all registered file types
	{
		auto io = ioFactory->create();
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
		if ( (io->supportedDataSetTypes() & allowedTypes) == 0 )
#else
		if (!io->supportedDataSetTypes().testAnyFlags(allowedTypes))
#endif
		{   // current I/O does not support any of the allowed types
			continue;
		}
		auto extCpy = io->extensions();
		for (auto & ext: extCpy)
		{
			ext = "*." + ext;
		}
		singleTypes += QString("%1 (%2);;").arg(io->name()).arg(extCpy.join(" "));
		allExtensions.append(extCpy);
	}
	if (singleTypes.isEmpty())
	{
		LOG(lvlWarn, "No matching registered file types found!");
		return ";;";
	}
	return QString("Any supported format (%1);;").arg(allExtensions.join(" ")) + singleTypes;
}

// ---------- specific File IO's ----------

class iAMetaFileIO : public iAFileIO
{
public:
	iAMetaFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iAVTIFileIO : public iAFileIO
{
public:
	iAVTIFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iAGraphFileIO : public iAFileIO
{
public:
	iAGraphFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iASTLFileIO : public iAFileIO
{
public:
	iASTLFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iAAmiraVolumeFileIO : public iAFileIO
{
public:
	iAAmiraVolumeFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iARawFileIO : public iAFileIO
{
public:
	iARawFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

// ---------- iAFileTypeRegistry::setupDefaultIOFactories (needs to be after declaration of specific IO classes) ----------

void iAFileTypeRegistry::setupDefaultIOFactories()
{
	iAFileTypeRegistry::addFileType<iAMetaFileIO>();
	iAFileTypeRegistry::addFileType<iAVTIFileIO>();
	iAFileTypeRegistry::addFileType<iAAmiraVolumeFileIO>();
	iAFileTypeRegistry::addFileType<iARawFileIO>();

	iAFileTypeRegistry::addFileType<iAGraphFileIO>();

	iAFileTypeRegistry::addFileType<iASTLFileIO>();
}

// ---------- iAMetaFileIO ----------

#include "defines.h"
#include "iADataSet.h"
#include <iAFileUtils.h>
#include "iAProgress.h"
//#include "iAToolsVTK.h"

#include <vtkImageData.h>
#include <vtkMetaImageReader.h>

#include <QElapsedTimer>
#include <QFileInfo>

iAMetaFileIO::iAMetaFileIO() :
	iAFileIO(iADataSetType::Volume)
{}

std::shared_ptr<iADataSet> iAMetaFileIO::load(iAProgress* p, QMap<QString, QVariant> const& parameters)
{
	Q_UNUSED(parameters);
	QElapsedTimer t;
	t.start();
	/*
	// using iAToolsVTK:
	auto img = vtkSmartPointer<vtkImageData>::New();
	readImage(m_fileName, true, img);
	// duration: ~400ms
	*/
	vtkNew<vtkMetaImageReader> reader;
	p->observe(reader);
	//reader->SetFileName(m_fileName.toStdString().c_str());
	reader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	reader->Update();
	reader->ReleaseDataFlagOn();
	auto img = reader->GetOutput();
	// duration: 362,362,368,368,383 ms

	auto ret = std::make_shared<iAImageData>(QFileInfo(m_fileName).baseName(), m_fileName, img);
	LOG(lvlInfo, QString("Elapsed: %1 ms.").arg(t.elapsed()));
	return ret;
}

QString iAMetaFileIO::name() const
{
	return "Meta Image";
}

QStringList iAMetaFileIO::extensions() const
{
	return QStringList{"mhd", "mha"};
}

// ---------- iAGraphFileIO ----------

#include <QColor>

#include <vtkCellData.h>
#include <vtkLine.h>
#include <vtkPolyData.h>

#include "iAAABB.h"

iAGraphFileIO::iAGraphFileIO() : iAFileIO(iADataSetType::Graph)
{
	addParameter("Spacing", iAValueType::Vector3, QVariant::fromValue(QVector<double>{1.0, 1.0, 1.0}));
}

std::shared_ptr<iADataSet> iAGraphFileIO::load(iAProgress* p, QMap<QString, QVariant> const& params)
{
	// maybe we could also use vtkPDBReader, but not sure that's the right "PDB" file type...
	Q_UNUSED(p);

	auto spacing = params["Spacing"].value<QVector<double>>();

	vtkNew<vtkPolyData> myPolyData;

	QFile file(m_fileName);
	//const auto size = file.size();
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError,
			QString("Could not open file '%1' for reading! It probably does not exist!")
				.arg(m_fileName));
		return nullptr;
	}
	QStringList origCSVInfo;
	QTextStream in(&file);
	// skip headers:
	for (size_t r = 0; r < 4; ++r)
	{
		in.readLine();
	}

	// read vertices
	vtkNew<vtkUnsignedCharArray> colors;
	colors->SetNumberOfComponents(3);
	colors->SetName("Colors");
	vtkNew<vtkPoints> pts;
	//vtkNew<vtkIdList> pointIds;
	//vtkNew<vtkCellArray> polyPoint;
	//size_t curVert = 0;
	QString line = "";
	int numberOfPoints = 0;

	iAAABB bbox;
	while (!in.atEnd() && line != "$$")
	{
		line = in.readLine();
		auto tokens = line.split("\t");
		if (tokens.size() == 7)
		{
			iAVec3d pos(
				tokens[2].toDouble() * spacing[0],
				tokens[3].toDouble() * spacing[1],
				tokens[4].toDouble() * spacing[2]
			);
			bbox.addPointToBox(pos);
			pts->InsertNextPoint(pos.data());
			QColor color(tokens[5]);
			//pointIds->InsertNextId(curVert);
			//polyPoint->InsertNextCell(pointIds);
			unsigned char c[3] = {static_cast<unsigned char>(color.red()), static_cast<unsigned char>(color.green()),
				static_cast<unsigned char>(color.blue())};
			colors->InsertNextTypedTuple(c);
			++numberOfPoints;
		}
		//auto remains = file.bytesAvailable();
		//auto progress = ((size - remains) * 100) / size;
	}
	assert(numberOfPoints == pts->GetNumberOfPoints());
	
	// some axes are flipped in comparison to our image data:
	for (int i = 0; i < numberOfPoints; ++i)
	{
		double pt[3];
		pts->GetPoint(i, pt);
		std::swap(pt[0], pt[1]);
		//pt[0] = bbox.bottomRight().x() - pt[0];
		//pt[1] = bbox.bottomRight().y() - pt[1];
		pts->SetPoint(i, pt);
	}
	

	myPolyData->SetPoints(pts);
	LOG(lvlInfo, QString("%1 points in box %3").arg(pts->GetNumberOfPoints()).arg(toStr(bbox)));

	//myPolyData->SetVerts(polyPoint);
	//myPolyData->GetCellData()->SetScalars(colors);

	line = "";
	in.readLine();    // skip header

	// read edges
	vtkNew<vtkCellArray> lines;
	size_t numberOfLines = 0;
	while (!in.atEnd() && line != "$$")
	{
		line = in.readLine();
		auto tokens = line.split("\t");
		if (tokens.size() == 4)
		{

			vtkNew<vtkLine> lineNEW;
			bool ok;
			int pt1 = tokens[1].toInt(&ok) - 1;
			if (!ok || pt1 < 0 || pt1 >= pts->GetNumberOfPoints())
			{
				LOG(lvlInfo, QString("Invalid point index 1 in line %1: %2").arg(line).arg(pt1));
			}
			int pt2 = tokens[2].toInt(&ok)-1;
			if (!ok || pt2 < 0 || pt2 >= pts->GetNumberOfPoints())
			{
				LOG(lvlInfo, QString("Invalid point index 2 in line %1: %2").arg(line).arg(pt2));
			}
			lineNEW->GetPointIds()->SetId(0, pt1);
			lineNEW->GetPointIds()->SetId(1, pt2);

			//LOG(lvlInfo, QString("inserting line : %1 -> %2").arg(pt1).arg(pt2));
			lines->InsertNextCell(lineNEW);
			++numberOfLines;
		}
		//auto remains = file.bytesAvailable();
		//auto progress = ((size - remains) * 100) / size;
	}
	//LOG(lvlInfo, QString("Number of lines: %1").arg(numberOfLines));

	// skip last section for now

	myPolyData->SetLines(lines);
	myPolyData->GetPointData()->AddArray(colors);

	return std::make_shared<iAGraphData>(QFileInfo(m_fileName).baseName(), m_fileName, myPolyData);
}

QString iAGraphFileIO::name() const
{
	return "Graph file";
}

QStringList iAGraphFileIO::extensions() const
{                             // pdb as in Brookhaven "Protein Data Bank" format (?)
	return QStringList{"txt", "pdb"};
}

// ---------- iASTLFileIO ----------

#include <iAFileUtils.h>

iASTLFileIO::iASTLFileIO() : iAFileIO(iADataSetType::Mesh)
{
}

std::shared_ptr<iADataSet> iASTLFileIO::load(iAProgress* p, QMap<QString, QVariant> const& params)
{
	Q_UNUSED(params);
	auto stlReader = vtkSmartPointer<vtkSTLReader>::New();
	p->observe(stlReader);
	stlReader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	vtkNew<vtkPolyData> polyData;
	stlReader->SetOutput(polyData);
	stlReader->Update();
	return std::make_shared<iAPolyData>(QFileInfo(m_fileName).baseName(), m_fileName, polyData);
}

QString iASTLFileIO::name() const
{
	return "STL file";
}

QStringList iASTLFileIO::extensions() const
{
	return QStringList{"stl"};
}


// ---------- iAVTIFileIO ----------

#include <vtkXMLImageDataReader.h>

iAVTIFileIO::iAVTIFileIO() : iAFileIO(iADataSetType::Volume)
{
}

std::shared_ptr<iADataSet> iAVTIFileIO::load(iAProgress* p, QMap<QString, QVariant> const& parameters)
{
	Q_UNUSED(parameters);
	QElapsedTimer t;
	t.start();

	auto reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
	p->observe(reader);
	reader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	reader->Update();
	reader->ReleaseDataFlagOn();
	auto img = reader->GetOutput();

	auto ret = std::make_shared<iAImageData>(QFileInfo(m_fileName).baseName(), m_fileName, img);
	LOG(lvlInfo, QString("Elapsed: %1 ms.").arg(t.elapsed()));
	return ret;
}

QString iAVTIFileIO::name() const
{
	return "Serial XML VTK image data";
}

QStringList iAVTIFileIO::extensions() const
{
	return QStringList{"vti"};
}


// ---------- iAAmiraVolumeFileIO ----------

#include "iAAmiraMeshIO.h"

iAAmiraVolumeFileIO::iAAmiraVolumeFileIO() : iAFileIO(iADataSetType::Volume)
{
}

std::shared_ptr<iADataSet> iAAmiraVolumeFileIO::load(iAProgress* p, QMap<QString, QVariant> const& parameters)
{
	Q_UNUSED(p);
	Q_UNUSED(parameters);
	QElapsedTimer t;
	t.start();

	auto img = iAAmiraMeshIO::Load(m_fileName);

	auto ret = std::make_shared<iAImageData>(QFileInfo(m_fileName).baseName(), m_fileName, img);
	LOG(lvlInfo, QString("Elapsed: %1 ms.").arg(t.elapsed()));
	return ret;
}

QString iAAmiraVolumeFileIO::name() const
{
	return "Amira volume data";
}

QStringList iAAmiraVolumeFileIO::extensions() const
{
	return QStringList{"am"};
}


// ---------- iARawFileIO ---------

#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...

#define RAW_LOAD_WAY ITK

#if RAW_LOAD_WAY == ITK
#include "iAConnector.h"
#include "iARawFileParameters.h"
#include "iATypedCallHelper.h"

#include <itkImage.h>
#include <itkRawImageIO.h>
#include <itkImageFileReader.h>
#else // VTK
#include <vtkImageReader2.h>
#endif



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

#if RAW_LOAD_WAY == ITK
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

#if RAW_LOAD_WAY == ITK
	VTK_TYPED_CALL(read_raw_image_template, rawFileParams.m_scalarType, rawFileParams, m_fileName, p, con);
	// we need a deep copy here, this causes a crash further down the line:
	// auto img = con.vtkImage();
	// instead:
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
	// VTK: 1024, 1257, 1026, 1011 (1038), 1014 (1034) ms (check if that includes computing min/max!!!
	// ITK: 173, 170, 163, 159, 160 ms
	//      165 (181), 165 (182), 162 (179) ms
	// Quick tests with CFK-Probe_Stahlstift_10xavg_freebeam_448proj.raw (~5GB):
	// VTK:  6690 (6956) ms
	// ITK: 51837 (52087) ms (erstes Lesen von Hard disk), 5463 (5721) ms (zweites lesen, nach VTK)
#endif

	auto ret = std::make_shared<iAImageData>(QFileInfo(m_fileName).baseName(), m_fileName, img);
	LOG(lvlInfo, QString("Elapsed: %1 ms.").arg(t.elapsed()));
	// TODO: maybe compute range here as well?
	//auto rng = img->GetScalarRange();
	//LOG(lvlInfo, QString("After min/max: %1 ms.").arg(t.elapsed()));
	return ret;
}

QString iARawFileIO::name() const
{
	return "RAW files";
}

QStringList iARawFileIO::extensions() const
{
	return QStringList{"raw", "vol", "rec", "pro"};
}



namespace iANewIO
{
	std::shared_ptr<iAFileIO> createIO(QString fileName)
	{
		QFileInfo fi(fileName);
		// special handling for directory ? TLGICT-loader... -> fi.isDir();
		auto io = iAFileTypeRegistry::createIO(fi.suffix());
		if (!io)
		{
			LOG(lvlWarn,
				QString("Failed to load %1: There is no handler registered files with suffix '%2'")
					.arg(fileName)
					.arg(fi.suffix()));
			return {};
		}
		io->setup(fileName);
		// for file formats that support multiple dataset types: check if an allowed type was loaded?
		// BUT currently no such format supported
		return io;
	}
}
