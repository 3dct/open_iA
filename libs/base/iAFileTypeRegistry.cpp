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

#include "iAFileIO.h"

#include <vtkPointData.h>
#include <vtkSTLReader.h>
#include <vtkUnsignedCharArray.h>

#include <QTextStream>

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
	std::vector<std::shared_ptr<iADataSet>> load(iAProgress* p, QVariantMap const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iAVTIFileIO : public iAFileIO
{
public:
	iAVTIFileIO();
	std::vector<std::shared_ptr<iADataSet>> load(iAProgress* p, QVariantMap const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iAGraphFileIO : public iAFileIO
{
public:
	iAGraphFileIO();
	std::vector<std::shared_ptr<iADataSet>> load(iAProgress* p, QVariantMap const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iASTLFileIO : public iAFileIO
{
public:
	iASTLFileIO();
	std::vector<std::shared_ptr<iADataSet>> load(iAProgress* p, QVariantMap const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

class iAAmiraVolumeFileIO : public iAFileIO
{
public:
	iAAmiraVolumeFileIO();
	std::vector<std::shared_ptr<iADataSet>> load(iAProgress* p, QVariantMap const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

// ---------- iAFileTypeRegistry::setupDefaultIOFactories (needs to be after declaration of specific IO classes) ----------

#include "iAHDF5IO.h"
#include "iAProjectFileIO.h"
#include "iARawFileIO.h"

void iAFileTypeRegistry::setupDefaultIOFactories()
{
	// volume file formats:
	iAFileTypeRegistry::addFileType<iAMetaFileIO>();
	iAFileTypeRegistry::addFileType<iAVTIFileIO>();
	iAFileTypeRegistry::addFileType<iAAmiraVolumeFileIO>();
	iAFileTypeRegistry::addFileType<iARawFileIO>();
#ifdef USE_HDF5
	iAFileTypeRegistry::addFileType<iAHDF5IO>();
#endif
	// mesh file formats:

	iAFileTypeRegistry::addFileType<iASTLFileIO>();

	// graph file formats:
	iAFileTypeRegistry::addFileType<iAGraphFileIO>();
	
	// collection file formats:
	iAFileTypeRegistry::addFileType<iAProjectFileIO>();
}

// ---------- iAMetaFileIO ----------

#include "defines.h"
#include "iADataSet.h"
#include "iAFileUtils.h"
#include "iAProgress.h"

#define VTK 0
#define ITK 1

#define META_LOAD_METHOD VTK
#if META_LOAD_METHOD == VTK
#include <vtkImageData.h>
#include <vtkMetaImageReader.h>
#else
#include "iAToolsVTK.h"
#endif

iAMetaFileIO::iAMetaFileIO() :
	iAFileIO(iADataSetType::Volume)
{}

std::vector<std::shared_ptr<iADataSet>> iAMetaFileIO::load(iAProgress* p, QVariantMap const& parameters)
{
	Q_UNUSED(parameters);

#if META_LOAD_METHOD == VTK
	vtkNew<vtkMetaImageReader> reader;
	p->observe(reader);
	//reader->SetFileName(m_fileName.toStdString().c_str());
	reader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	reader->Update();
	reader->ReleaseDataFlagOn();
	auto img = reader->GetOutput();
	// duration: 362,362,368,368,383 ms
#else
	Q_UNUSED(p);
	auto img = vtkSmartPointer<vtkImageData>::New();
	readImage(m_fileName, true, img);    //< using iAToolsVTK
	// duration: ~400ms
#endif

	// new tests with large dataset (D:\TestDatasets\large\CFK-Probe_Stahlstift_10xavg_freebeam_448proj.raw):
	//
	// VTK: 50571(ignore, caching...), 4484, 4477, 4529 ms
	// ITK (using readImage from iAToolsVTK): 5876 (ignore), 5877, 5760, 5746 ms
	// -> VTK consistently faster!

	return { std::make_shared<iAImageData>(m_fileName, img) };
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

#include "iAAABB.h"
#include "iAValueTypeVectorHelpers.h"

#include <vtkCellData.h>
#include <vtkLine.h>
#include <vtkPolyData.h>

#include <QColor>
#include <QFile>

iAGraphFileIO::iAGraphFileIO() : iAFileIO(iADataSetType::Graph)
{
	addParameter("Spacing", iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
}

std::vector<std::shared_ptr<iADataSet>> iAGraphFileIO::load(iAProgress* p, QVariantMap const& params)
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
		return {};
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

	return { std::make_shared<iAGraphData>(m_fileName, myPolyData) };
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

std::vector<std::shared_ptr<iADataSet>> iASTLFileIO::load(iAProgress* p, QVariantMap const& params)
{
	Q_UNUSED(params);
	auto stlReader = vtkSmartPointer<vtkSTLReader>::New();
	p->observe(stlReader);
	stlReader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	vtkNew<vtkPolyData> polyData;
	stlReader->SetOutput(polyData);
	stlReader->Update();
	return { std::make_shared<iAPolyData>(m_fileName, polyData) };
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

std::vector<std::shared_ptr<iADataSet>> iAVTIFileIO::load(iAProgress* p, QVariantMap const& parameters)
{
	Q_UNUSED(parameters);
	auto reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
	p->observe(reader);
	reader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	reader->Update();
	reader->ReleaseDataFlagOn();
	auto img = reader->GetOutput();
	return { std::make_shared<iAImageData>(m_fileName, img) };
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

std::vector<std::shared_ptr<iADataSet>> iAAmiraVolumeFileIO::load(iAProgress* p, QVariantMap const& parameters)
{
	Q_UNUSED(p);
	Q_UNUSED(parameters);
	auto img = iAAmiraMeshIO::Load(m_fileName);
	return { std::make_shared<iAImageData>(m_fileName, img) };
}

QString iAAmiraVolumeFileIO::name() const
{
	return "Amira volume data";
}

QStringList iAAmiraVolumeFileIO::extensions() const
{
	return QStringList{"am"};
}

#include <QFileInfo>

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
