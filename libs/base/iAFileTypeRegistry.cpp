#include "iAFileTypeRegistry.h"

#include <vtkPointData.h>
#include <vtkSTLReader.h>
#include <vtkUnsignedCharArray.h>

#include <QTextStream>

// ---------- iAFileIO ----------

iAFileIO::iAFileIO(iADataSetType type): m_type(type)
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

iADataSetType iAFileIO::type() const
{
	return m_type;
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
		if (!allowedTypes.testFlag(io->type()))
		{
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

// ---------- iAFileTypeRegistry::setupDefaultIOFactories (needs to be after declaration of specific IO classes) ----------

void iAFileTypeRegistry::setupDefaultIOFactories()
{
	iAFileTypeRegistry::addFileType<iAMetaFileIO>();
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
	iAFileIO(iADataSetType::dstVolume)
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

iAGraphFileIO::iAGraphFileIO() : iAFileIO(iADataSetType::dstMesh)
{
	addParameter("Spacing X", iAValueType::Continuous, 1.0);
	addParameter("Spacing Y", iAValueType::Continuous, 1.0);
	addParameter("Spacing Z", iAValueType::Continuous, 1.0);
}

std::shared_ptr<iADataSet> iAGraphFileIO::load(iAProgress* p, QMap<QString, QVariant> const& params)
{
	// maybe we could also use vtkPDBReader, but not sure that's the right "PDB" file type...
	Q_UNUSED(p);

	double spacing[3] = {
		params["Spacing X"].toDouble(),
		params["Spacing Y"].toDouble(),
		params["Spacing Z"].toDouble()
	};

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

iASTLFileIO::iASTLFileIO() : iAFileIO(iADataSetType::dstMesh)
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


namespace iANewIO
{
	std::shared_ptr<iAFileIO> createIO(QString fileName, iADataSetTypes allowedTypes)
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
		// TODO: extend type check in io / only try loaders for allowedTypes (but how to handle file types that can contain multiple?)
		if (!allowedTypes.testFlag(io->type()))
		{
			LOG(lvlWarn,
				QString("Failed to load %1: The loaded dataset type %2 does not match any allowed type.")
					.arg(fileName)
					.arg(io->type()));
			return {};
		}
		return io;
	}
}
