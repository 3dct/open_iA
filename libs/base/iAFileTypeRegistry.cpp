#include "iAFileTypeRegistry.h"

#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>

#include <QTextStream>

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


QMap<QString, std::shared_ptr<iAIFileIOFactory>> iAFileTypeRegistry::m_fileTypes;

std::shared_ptr<iAFileIO> iAFileTypeRegistry::createIO(QString const& fileExtension)
{
	if (m_fileTypes.contains(fileExtension))
	{
		return m_fileTypes[fileExtension]->create();
	}
	else
	{
		return std::shared_ptr<iAFileIO>();
	}
}

#include "defines.h"
#include "iAConnector.h"
#include "iAToolsITK.h"
#include "iADataSet.h"
#include "iAExtendedTypedCallHelper.h"
#include "iAFileUtils.h"
#include "iAProgress.h"

#include <vtkImageData.h>
#include <vtkPolyData.h>

#include <itkImageFileReader.h>
#include <itkImageIOBase.h>
#include <itkImageIOFactory.h>

#include <QFileInfo>

template <class T>
void read_image_template(QString const& fileName, iAProgress* progress, iAConnector& con)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	auto reader = ReaderType::New();
	reader->SetFileName(getLocalEncodingFileName(fileName));
	reader->ReleaseDataFlagOn();
	progress->observe(reader);
	reader->Update();
	con.setImage(reader->GetOutput());
	con.modified();
}

iAITKFileIO::iAITKFileIO() :
	iAFileIO(iADataSetType::dstVolume)
{}

std::shared_ptr<iADataSet> iAITKFileIO::load(iAProgress* p, QMap<QString, QVariant> const& parameters)
{
	Q_UNUSED(parameters);
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	auto imageIO = itk::ImageIOFactory::CreateImageIO(
		getLocalEncodingFileName(m_fileName).c_str(), itk::ImageIOFactory::ReadMode);
	if (!imageIO)
		throw std::invalid_argument("Could not find a reader that could handle the format of the specified file!");
	imageIO->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	imageIO->ReadImageInformation();
	const ScalarPixelType pixelType = imageIO->GetComponentType();
	const PixelType imagePixelType = imageIO->GetPixelType();
	iAConnector con;
	ITK_EXTENDED_TYPED_CALL(read_image_template, pixelType, imagePixelType, m_fileName, p, con);
	storeImage(con.itkImage(), "C:/fh/testnewio2.mhd", false);
	return std::make_shared<iAImageData>(QFileInfo(m_fileName).baseName(), m_fileName, con.vtkImage());
}

/*
std::shared_ptr<iAFileIO> iAITKFileIO::create()
{
	return std::make_shared<iAITKFileIO>();
}
*/

//#include <vtkPDBReader.h>
#include <QColor>

#include <vtkCellData.h>
#include <vtkLine.h>

// TODO: move to separate iAAABB class and re-use in FIAKER/FiberSA
#include <iAVec3.h>
class iAAABB
{
public:
	iAAABB()
	{
		box[0].fill(std::numeric_limits<double>::max());
		box[1].fill(std::numeric_limits<double>::lowest());
	}
	void addPointToBox(iAVec3d const & pt)
	{
		for (int i = 0; i < 3; ++i)
		{
			box[0][i] = std::min(box[0][i], pt[i]);
			box[1][i] = std::max(box[1][i], pt[i]);
		}
		
	}
	iAVec3d const & topLeft() const
	{
		return box[0];
	}
	iAVec3d const& bottomRight() const
	{
		return box[1];
	}

private:
	std::array<iAVec3d, 2> box;
};

QString toStr(iAAABB const & box)
{
	return QString("%1, %2; %3, %4").arg(box.topLeft().x()).arg(box.topLeft().y()).arg(box.bottomRight().x()).arg(box.bottomRight().y());
}

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