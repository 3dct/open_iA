#include "iAFileTypeRegistry.h"

#include <QFileInfo>


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

std::unique_ptr<iADataSet> iAITKFileIO::load(QString const& fileName, iAProgress* p)
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
	return std::make_unique<iADataSet>(
		dstVolume, QFileInfo(fileName).baseName(), fileName, con.vtkImage(), nullptr);
}

/*
std::shared_ptr<iAFileIO> iAITKFileIO::create()
{
	return std::make_shared<iAITKFileIO>();
}
*/