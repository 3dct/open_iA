#include "iAFileTypeRegistry.h"

#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>

#include <QFileInfo>
#include <QTextStream>


iAFileIO::~iAFileIO()
{}


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

iADataSet* iAITKFileIO::load(QString const& fileName, iAProgress* p)
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	auto imageIO = itk::ImageIOFactory::CreateImageIO(
		getLocalEncodingFileName(fileName).c_str(), itk::ImageIOFactory::ReadMode);
	if (!imageIO)
		throw std::invalid_argument("Could not find a reader that could handle the format of the specified file!");
	imageIO->SetFileName(getLocalEncodingFileName(fileName).c_str());
	imageIO->ReadImageInformation();
	const ScalarPixelType pixelType = imageIO->GetComponentType();
	const PixelType imagePixelType = imageIO->GetPixelType();
	iAConnector con;
	ITK_EXTENDED_TYPED_CALL(read_image_template, pixelType, imagePixelType, fileName, p, con);
	storeImage(con.itkImage(), "C:/fh/testnewio2.mhd", false);
	return new iADataSet(
		dstVolume, QFileInfo(fileName).baseName(), fileName, con.vtkImage(), nullptr);
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

iADataSet* iAGraphFileIO::load(QString const& fileName, iAProgress* p)
{
	//vtkNew<vtkPDBReader> reader;
	//reader->SetInput
	Q_UNUSED(p);
	QFile file(fileName);
	//const auto size = file.size();
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError,
			QString("Could not open CSV file '%1' for reading! "
					"It probably does not exist!")
				.arg(fileName));
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
	while (!in.atEnd() && line != "$$")
	{
		line = in.readLine();
		auto tokens = line.split("\t");
		if (tokens.size() == 7)
		{
			double pos[3] = {
				tokens[2].toDouble(),
				tokens[3].toDouble(),
				tokens[4].toDouble(),
			};
			pts->InsertNextPoint(pos);
			QColor color(tokens[5]);
			//pointIds->InsertNextId(curVert);
			//polyPoint->InsertNextCell(pointIds);
			unsigned char c[3] = {static_cast<unsigned char>(color.red()), static_cast<unsigned char>(color.green()),
				static_cast<unsigned char>(color.blue())};
			colors->InsertNextTypedTuple(c);
		}
		//auto remains = file.bytesAvailable();
		//auto progress = ((size - remains) * 100) / size;
	}
	vtkNew<vtkPolyData> myPolyData;
	myPolyData->SetPoints(pts);
	//myPolyData->SetVerts(polyPoint);
	//myPolyData->GetCellData()->SetScalars(colors);

	line = "";

	// read edges
	vtkNew<vtkCellArray> lines;
	while (!in.atEnd() && line != "$$")
	{
		line = in.readLine();
		auto tokens = line.split("\t");
		if (tokens.size() == 4)
		{

			vtkNew<vtkLine> lineNEW;
			lineNEW->GetPointIds()->SetId(0, tokens[1].toInt());
			lineNEW->GetPointIds()->SetId(1, tokens[2].toInt());
			lines->InsertNextCell(lineNEW);
		}
		//auto remains = file.bytesAvailable();
		//auto progress = ((size - remains) * 100) / size;
	}

	// skip last section for now

	myPolyData->SetLines(lines);
	myPolyData->GetPointData()->AddArray(colors);

	// HEAP CORRUPTION - try with small example vtkPolyData?

	return new iADataSet(dstMesh, QFileInfo(fileName).baseName(), fileName, nullptr, myPolyData);
}