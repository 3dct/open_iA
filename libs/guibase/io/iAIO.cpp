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
#include "iAIO.h"

// guibase
#include "iAJobListView.h"
#include "iAMainWindow.h"    // TODO: check if it can be avoided
#include "iAModalityList.h"
#include "iAParameterDlg.h"
#include "iARawFileParamDlg.h"

// base
#include "defines.h"
#include "iAConnector.h"
#include "iALog.h"
#include "iAExceptionThrowingErrorObserver.h"
#include "iAExtendedTypedCallHelper.h"
#include "iAFileUtils.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAOIFReader.h"
#include "iAProgress.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"

#include <itkBMPImageIO.h>
#include <itkMacro.h>    // for itkExceptionObject, which (starting with ITK 5.1), may not be included directly
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOBase.h>
#include <itkImageSeriesReader.h>
#include <itkImageSeriesWriter.h>
#include <itkJPEGImageIO.h>
#include <itkNumericSeriesFileNames.h>
#include <itkPNGImageIO.h>
#include <itkRawImageIO.h>
#include <itkTIFFImageIO.h>

#include <vtkBMPReader.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkJPEGReader.h>
#include <vtkPNGReader.h>
#include <vtkPolyData.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTIFFReader.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkRectilinearGrid.h>
#include <vtkPointData.h>

#include <QApplication>
#include <QFileDialog>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QTextStream>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <QMap>

mapQString2int const & extensionToId()
{
	static mapQString2int m;
	if (m.empty())
	{
		m[""] = UNKNOWN_READER;
		m["MHD"] = MHD_READER;
		m["MHA"] = MHD_READER;
		m["STL"] = STL_READER;
		m["RAW"] = RAW_READER;
		m["VOL"] = RAW_READER;
		m["REC"] = RAW_READER;
		m["PRO"] = RAW_READER;
		m["PARS"] = PARS_READER;
		m["VGI"] = VGI_READER;
		m["NKC"] = NKC_READER;
		m["DCM"] = DCM_READER;
		//	m["NRRD"] = NRRD_READER; see iAIOProvider.cpp why this is commented out
		//	m["NHDR"] = NRRD_READER;
		m["TIF"] = MHD_READER;
		m["TIFF"] = MHD_READER;
		m["JPG"] = MHD_READER;
		m["JPEG"] = MHD_READER;
		m["PNG"] = MHD_READER;
		m["BMP"] = MHD_READER;
		m["NIA"] = MHD_READER;
		m["NII"] = MHD_READER;
		m["GZ"] = MHD_READER;  // actually, only nii.gz and img.gz...
		m["HDR"] = MHD_READER;
		m["IMG"] = MHD_READER;
		m["OIF"] = OIF_READER;
		m["VTK"] = VTK_READER;
		m["MOD"] = PROJECT_READER;
		m["IAPROJ"] = PROJECT_READER;
	}
	return m;
}

mapQString2int const & extensionToIdStack()
{
	static mapQString2int m;
	if (m.empty())
	{
		m[""] = UNKNOWN_READER;
		m["RAW"] = VOLUME_STACK_READER;
		m["MHD"] = VOLUME_STACK_MHD_READER;
		m["VOLSTACK"] = VOLUME_STACK_VOLSTACK_READER;
		m["TIF"] = TIF_STACK_READER;
		m["TIFF"] = TIF_STACK_READER;
		m["JPG"] = JPG_STACK_READER;
		m["JPEG"] = JPG_STACK_READER;
		m["PNG"] = PNG_STACK_READER;
		m["BMP"] = BMP_STACK_READER;
	}
	return m;
}

mapQString2int const & extensionToSaveId()
{
	static mapQString2int m;
	if (m.empty())
	{
		m["TIF"] = TIF_STACK_WRITER;
		m["TIFF"] = TIF_STACK_WRITER;
		m["JPG"] = JPG_STACK_WRITER;
		m["JPEG"] = JPG_STACK_WRITER;
		m["PNG"] = PNG_STACK_WRITER;
		m["BMP"] = BMP_STACK_WRITER;
		m["DCM"] = DCM_WRITER;
		m["CSV"] = CSV_WRITER;
		m["HDF5"] = MHD_WRITER;
		m["HE5"] = MHD_WRITER;
		m["H5"] = MHD_WRITER;
	}
	return m;
}

const QString iAIO::VolstackExtension(".volstack");

template<class T>
void read_raw_image_template (iARawFileParameters const & params,
	QString const & fileName, iAProgress* progress, iAConnector* image)
{
	typedef itk::RawImageIO<T, DIM> RawImageIOType;
	auto io = RawImageIOType::New();
	io->SetFileName(getLocalEncodingFileName(fileName).c_str());
	io->SetHeaderSize(params.m_headersize);
	for(int i=0; i<DIM; i++)
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

	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	auto reader = ReaderType::New();
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	reader->SetImageIO(io);
	progress->observe( reader );
	reader->Modified();
	reader->Update();
	image->setImage(reader->GetOutput());
	image->modified();
	reader->ReleaseDataFlagOn();
}

template<class T>
void read_image_template(QString const & fileName, iAProgress* progress, iAConnector* con  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	auto reader = ReaderType::New();
	reader->SetFileName( getLocalEncodingFileName(fileName) );
	progress->observe( reader );
	reader->Update();
	con->setImage(reader->GetOutput());
	con->modified();
	reader->ReleaseDataFlagOn();
}

template<class T>
void write_image_template(bool compression, QString const & fileName,
	iAProgress* progress, iAConnector* image  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileWriter<InputImageType> WriterType;
	auto writer = WriterType::New();
	writer->SetFileName(getLocalEncodingFileName(fileName).c_str());
	writer->SetInput( dynamic_cast< InputImageType * > ( image->itkImage() ) );
	writer->SetUseCompression(compression);
	progress->observe( writer );
	writer->Update();
	writer->ReleaseDataFlagOn();
}

iAIO::iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QWidget *par,
		std::vector<vtkSmartPointer<vtkImageData> > * volumes, std::vector<QString> * fileNames)
	: iAAlgorithm( "IO", i, p, logger, par ),
	m_volumes(volumes),
	m_fileNames_volstack(fileNames)
{
	init(par);
}

iAIO::iAIO( iALogger* logger, QWidget *par /*= 0*/, std::vector<vtkSmartPointer<vtkImageData> > * volumes /*= 0*/,
		std::vector<QString> * fileNames /*= 0*/ )
	: iAAlgorithm( "IO", 0, 0, logger, par),
	m_volumes(volumes),
	m_fileNames_volstack(fileNames)
{
	init(par);
}

void iAIO::init(QWidget *par)
{
	m_parent = par;
	m_fileName = "";
	m_fileNameArray = vtkStringArray::New();
	m_ioID = 0;
	loadIOSettings();
}

iAIO::~iAIO()
{
	m_fileNameArray->Delete();
}

namespace
{
	QString const FileNameBase("File name base");
	QString const Extension   ("Extension");
	QString const NumDigits   ("Number of digits in index");
	QString const MinimumIndex("Minimum index");
	QString const MaximumIndex("Maximum index");

	void addSeriesParameters(iAAttributes& params, QString const& base, QString const& ext, int digits, int const * index)
	{
		addAttr(params, FileNameBase, iAValueType::String, base);
		addAttr(params, Extension, iAValueType::String, ext);
		addAttr(params, NumDigits, iAValueType::Discrete, digits);
		addAttr(params, MinimumIndex, iAValueType::Discrete, index[0]);
		addAttr(params, MaximumIndex, iAValueType::Discrete, index[1]);
	}
}

void iAIO::readProject()
{
	m_modalities = QSharedPointer<iAModalityList>::create();
	m_modalities->load(m_fileName, *ProgressObserver());
}

void iAIO::run()
{
	QApplication::processEvents();
	try
	{
		switch (m_ioID)
		{
			case MHD_WRITER:
				writeMetaImage(getVtkImageData(), m_fileName); break;
			case VOLUME_STACK_VOLSTACK_WRITER:
				writeVolumeStack(); break;
			case STL_WRITER:
				writeSTL(); break;
			case TIF_STACK_WRITER:
			case JPG_STACK_WRITER:
			case PNG_STACK_WRITER:
			case BMP_STACK_WRITER:
			case DCM_WRITER:
				writeImageStack(); break;
			case MHD_READER:
				readMetaImage(); break;
			case STL_READER:
				readSTL(); break;
			case VTK_READER:
				readVTKFile(); break;
			case RAW_READER:
			case PARS_READER:
			case VGI_READER:
				readImageData(); break;
			case NKC_READER:
				readNKC(); break;
			case VOLUME_STACK_READER:
				readVolumeStack(); break;
			case VOLUME_STACK_MHD_READER:
			case VOLUME_STACK_VOLSTACK_READER:
				readVolumeMHDStack(); break;
			case TIF_STACK_READER:
			case JPG_STACK_READER:
			case PNG_STACK_READER:
			case BMP_STACK_READER:
				readImageStack(); break;
			case DCM_READER:
				readDCM(); break;
			//case NRRD_READER:
			//	readNRRD(); break;
			case OIF_READER: {
				readOIF(fileName(), getConnector(), m_channel, m_volumes);
				//if (!m_volumes)
				{
					postImageReadActions();
				}
				break;
			}
			/*
			case AM_WRITER: {
				iAAmiraMeshIO::Write(fileName(), getVtkImageData());
				break;
			}
			*/
			case CSV_WRITER: {
				// TODO: write more than one modality!
				auto img = getVtkImageData();
				int numberOfComponents = img->GetNumberOfScalarComponents();
				std::ofstream out( getLocalEncodingFileName(fileName()));
				FOR_VTKIMG_PIXELS(img, x, y, z)
				{
					for (int c = 0; c < numberOfComponents; ++c)
					{
						out << img->GetScalarComponentAsDouble(x, y, z, 0);
						if (c < numberOfComponents - 1)
						{
							out << ",";
						}
					}
					out << std::endl;
				}
				out.close();
				break;
			}
			case PROJECT_READER:
				readProject();
				break;
			case UNKNOWN_READER:
			default:
				addMsg(tr("Unknown reader type"));
		}
		if ((m_ioID == MHD_WRITER) || (m_ioID == STL_WRITER) || (m_ioID == TIF_STACK_WRITER)
			|| (m_ioID == JPG_STACK_WRITER) || (m_ioID == PNG_STACK_WRITER)|| (m_ioID == BMP_STACK_WRITER) || (m_ioID ==DCM_WRITER) )
			emit done(true);
		else emit done();
	}
	catch (std::exception & e)
	{
		LOG(lvlError, tr("IO operation failed: %1").arg(e.what()));
	}
}

bool iAIO::setupIO( iAIOType type, QString f, bool c, int channel, bool addJob)
{
	m_ioID = type;
	m_channel = channel;

	m_fileDir = QFileInfo(f).absoluteDir();

	if (addJob)
	{
		iAJobListView::get()->addJob(
			QString("%1 file(s)").arg((m_ioID >= MHD_WRITER) ? "Writing" : "Reading"), ProgressObserver(), this);
	}
	// TODO: hook for plugins!
	switch (m_ioID)
	{
		case MHD_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case STL_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case MHD_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case STL_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case VTK_READER:
			m_fileName = f; m_compression = c; break;
		case RAW_READER:
			return setupRAWReader(f);
		case PARS_READER:
			return setupPARSReader(f);
		case VGI_READER:
			return setupVGIReader(f);
		case NKC_READER:
			return setupNKCReader(f);
		case TIF_STACK_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case JPG_STACK_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case PNG_STACK_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case BMP_STACK_READER:
			return setupStackReader(f);
		case VOLUME_STACK_READER:
			return setupVolumeStackReader(f);
		case VOLUME_STACK_MHD_READER:
			return setupVolumeStackMHDReader(f);
		case VOLUME_STACK_VOLSTACK_READER:
			return setupVolumeStackVolstackReader(f);
		case VOLUME_STACK_VOLSTACK_WRITER:
			return setupVolumeStackVolStackWriter(f);

		case PROJECT_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case TIF_STACK_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case JPG_STACK_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case PNG_STACK_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case BMP_STACK_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case DCM_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case DCM_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		//case NRRD_READER:
		case OIF_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case AM_WRITER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case CSV_WRITER:

			m_fileName = f; break;
		case UNKNOWN_READER:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		default:
			LOG(lvlError, QString("Unknown IO type '%1' for file '%2'!").arg(m_ioID).arg(f));
			addMsg(tr("Unknown IO type"));
			return false;
	}
	return true;
}

/*
void iAIO::readNRRD()
{
	typedef itk::Vector<signed short, 2>	VectorType;
	typedef itk::Image<VectorType, DIM>		DiffusionImageType;
	typedef DiffusionImageType::Pointer		DiffusionImagePointer;
	typedef itk::ImageFileReader<DiffusionImageType> FileReaderType;
	auto reader = FileReaderType::New();
	reader->SetFileName( getLocalEncodingFileName(fileName) );
	reader->Update();
	itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
	io->SetFileType( itk::ImageIOBase::ASCII);
	iAConnector *con = getConnector();
	con->setImage(reader->GetOutput());
	con->modified();
	postImageReadActions();
	StoreIOSettings();
}
*/

void iAIO::readDCM()
{
	typedef signed short PixelType;
	typedef itk::Image<PixelType, DIM> ImageType;
	typedef itk::ImageSeriesReader<ImageType> ReaderType;
	auto reader = ReaderType::New();
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer dicomIO = ImageIOType::New();
	reader->SetImageIO( dicomIO );
	typedef itk::GDCMSeriesFileNames NamesGeneratorType;
	NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
	nameGenerator->SetUseSeriesDetails(true);
	nameGenerator->SetDirectory(getLocalEncodingFileName(m_fileDir.canonicalPath()));
	typedef std::vector<std::string> SeriesIdContainer;

	const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
	SeriesIdContainer::const_iterator seriesItr = seriesUID.begin();
	SeriesIdContainer::const_iterator seriesEnd = seriesUID.end();

	while (seriesItr != seriesEnd) ++seriesItr;

	std::string seriesIdentifier = seriesUID.begin()->c_str();

	typedef std::vector<std::string> FileNamesContainer;
	FileNamesContainer fileNames;
	fileNames = nameGenerator->GetFileNames(seriesIdentifier);

	reader->SetFileNames(fileNames);
	reader->Update();
	reader->Modified();
	reader->Update();

	iAConnector *image = getConnector();
	image->setImage(reader->GetOutput());
	image->modified();
	postImageReadActions();
	storeIOSettings();
}

void iAIO::loadMetaImageFile(QString const & fileName)
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	auto imageIO = itk::ImageIOFactory::CreateImageIO(getLocalEncodingFileName(fileName).c_str(), itk::ImageIOFactory::ReadMode);
	if (!imageIO)
		throw std::invalid_argument("Could not find a reader that could handle the format of the specified file!");
	imageIO->SetFileName(getLocalEncodingFileName(fileName).c_str());
	imageIO->ReadImageInformation();
	const ScalarPixelType pixelType = imageIO->GetComponentType();
	const PixelType imagePixelType = imageIO->GetPixelType();
	ITK_EXTENDED_TYPED_CALL(read_image_template, pixelType, imagePixelType,
		fileName, ProgressObserver(), getConnector());
}

void iAIO::readVTKFile()
{
	// Get all data from the file
	auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
	reader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	reader->Update();

	// All of the standard data types can be checked and obtained like this:
	if (reader->IsFilePolyData())
	{
		LOG(lvlInfo, "output is a polydata");

		getVtkPolyData()->DeepCopy(reader->GetPolyDataOutput());
		printSTLFileInfos();
		addMsg(tr("File loaded."));

	}
	else if (reader->IsFileRectilinearGrid())
	{
		addMsg(tr("output is reclinearGrid"));

		auto rectilinearGrid = reader->GetRectilinearGridOutput();
		int * extent = rectilinearGrid->GetExtent();
		vtkDataArray* coords[3] = {
			rectilinearGrid->GetXCoordinates(),
			rectilinearGrid->GetYCoordinates(),
			rectilinearGrid->GetZCoordinates()
		};

		const int NumDimensions = 3;
		double spacing[NumDimensions];
		// determine spacing and make sure it is the same over all coordinates:
		for (int i = 0; i < NumDimensions; ++i)
		{
			int numComp = coords[i]->GetNumberOfComponents();
			int numValues = coords[i]->GetNumberOfValues();
			int extentSize = extent[i * 2 + 1] - extent[i * 2] + 1;
			if (numComp != 1 || numValues != extentSize)
			{
				LOG(lvlWarn,
					QString("Don't know how to handle situation where number of components is %1 "
							"and number of values=%2 not equal to extentSize=%3")
						.arg(numComp)
						.arg(numValues)
						.arg(extentSize));
			}
			if (numValues < 2)
			{
				LOG(lvlWarn, QString("Dimension %1 has dimensions of less than 2, cannot compute proper spacing, using 1 instead!"));
				spacing[i] = 1;
			}
			else
			{
				spacing[i] = coords[i]->GetComponent(1, 0) - coords[i]->GetComponent(0, 0);
				for (int j = 2; j < numValues; ++j)
				{
					double actSpacing = coords[i]->GetComponent(j, 0) - coords[i]->GetComponent(j - 1, 0);
					if (actSpacing != spacing[i])
					{
						LOG(lvlWarn, QString("Spacing for cordinate %1 not the same as between 0..1 (%2) at index %3 (%4).")
							.arg(i)
							.arg(spacing[i])
							.arg(j)
							.arg(actSpacing));
					}
				}
			}
		}

		auto numOfArrays = rectilinearGrid->GetPointData()->GetNumberOfArrays();

//		for (
			int i = 0; // i < numOfArrays; ++i)
		if (numOfArrays > 0) //< remove this if enabling for loop
		{
			auto img = getVtkImageData();
			img->ReleaseData();
			img->Initialize();
			auto arrayData = rectilinearGrid->GetPointData()->GetAbstractArray(rectilinearGrid->GetPointData()->GetArrayName(i));
			int dataType = arrayData->GetDataType();

			img->SetExtent(extent);
			int size[3] = {
				extent[1] - extent[0],
				extent[3] - extent[2],
				extent[5] - extent[4],
			};
			img->SetSpacing(spacing);
			img->AllocateScalars(dataType, 1);
			//arrayData->
			// memcpy scalar pointer into img->GetScalarPointer ?
			size_t byteSize = mapVTKTypeToSize(dataType) * size[0] * size[1] * size[2];

			auto arrayPtr = arrayData->GetVoidPointer(0);
			std::memcpy(img->GetScalarPointer(), arrayPtr, byteSize);
			getConnector()->setImage(img);
			getConnector()->modified();

//			break; // in the future, when iAIO is refactored, load all datasets as separate modalities...
		}
		addMsg(tr("File loaded."));
	}
	else
	{
		addMsg("This type of vtk format is currently not supported");
	}
}

void iAIO::readVolumeMHDStack()
{
	if (m_fileNameArray->GetSize() == 0)
		throw std::runtime_error("No files matched the given criteria!");

	for (int m=0; m<= m_fileNameArray->GetMaxId(); m++)
	{
		m_fileName = QString::fromLocal8Bit(m_fileNameArray->GetValue(m).c_str());
		loadMetaImageFile(m_fileName);
		if (m_volumes)
		{
			vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
			image->DeepCopy(getConnector()->vtkImage());
			m_volumes->push_back(image);
		}
		if (m_fileNames_volstack)
			m_fileNames_volstack->push_back(m_fileName);

		double progress = (m_fileNameArray->GetMaxId() == 0) ? 100 : m * 100.0 / m_fileNameArray->GetMaxId();
		ProgressObserver()->emitProgress(progress);
	}
	addMsg(tr("Loading volume stack completed."));
	storeIOSettings();
}

void iAIO::readVolumeStack()
{
	for (int m=0; m<= m_fileNameArray->GetMaxId(); m++)
	{
		m_fileName=(m_fileNameArray->GetValue(m));
		readRawImage(false);
		vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
		image->DeepCopy(getConnector()->vtkImage());
		if (m_volumes)
		{
			m_volumes->push_back(image);
		}
		if (m_fileNames_volstack)
		{
			m_fileNames_volstack->push_back(m_fileName);
		}
		ProgressObserver()->emitProgress(m * 100.0 / m_fileNameArray->GetMaxId());
	}
	addMsg(tr("Loading volume stack completed."));
	storeIOSettings();
}

void iAIO::writeVolumeStack()
{
	// write .volstack file:
	QFile volstackFile(m_fileName);
	QFileInfo fi(m_fileName);
	size_t numOfVolumes = m_volumes->size();
	int numOfDigits = static_cast<int>(std::log10(static_cast<double>(m_volumes->size())) + 1);
	if(volstackFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&volstackFile);
		out << "file_names_base: " << fi.completeBaseName()	<< "\n"
			<< "extension: .mhd"							<< "\n"
			<< "number_of_digits_in_index: " << numOfDigits	<< "\n"
			<< "minimum_index: 0"							<< "\n"
			<< "maximum_index: " << numOfVolumes-1			<< "\n";
		if (!m_additionalInfo.isEmpty())
		{
			out << m_additionalInfo << "\n";
		}
	}
	// write mhd images:
	for (int m=0; m <= m_fileNameArray->GetMaxId(); m++)
	{
		writeMetaImage(m_volumes->at(m).GetPointer(), m_fileNameArray->GetValue(m).c_str());
		ProgressObserver()->emitProgress(m * 100.0 / m_fileNameArray->GetMaxId());
	}
}

void iAIO::readRawImage(bool reportProgress)
{
	iAProgress dummyProgress;
	VTK_TYPED_CALL(read_raw_image_template, m_rawFileParams.m_scalarType, m_rawFileParams, m_fileName,
		reportProgress ? ProgressObserver() : &dummyProgress, getConnector());
}

void iAIO::postImageReadActions()
{
	getVtkImageData()->ReleaseData();
	getVtkImageData()->Initialize();
	getVtkImageData()->DeepCopy(getConnector()->vtkImage());
	getVtkImageData()->CopyInformationFromPipeline(getConnector()->vtkImage()->GetInformation());
	addMsg(tr("File loaded."));
}

void iAIO::readImageData()
{
	readRawImage(true);
	postImageReadActions();
	storeIOSettings();
}

void iAIO::readNKC()
{
	readImageData();
	auto filter = iAFilterRegistry::filter("Replace and Shift");
	if (!filter)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Replace and Shift' filter, but filter could not be found!")
				.arg(m_fileName));
		return;
	}

	filter->addInput(getVtkImageData(), "");
	QVariantMap parameters;
	parameters["Value To Replace"] = 65533;
	parameters["Replacement"] = 0;
	filter->run(parameters);

	auto dataTypeConversion = iAFilterRegistry::filter("Datatype Conversion");
	if (!dataTypeConversion)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Datatype Conversion' filter, but filter could not be found!")
				.arg(m_fileName));
		return;
	}

	dataTypeConversion->addInput(filter->output(0)->itkImage(), "");
	QVariantMap parametersConversion;
	parametersConversion["Data Type"] = "32 bit floating point number (7 digits, float)";
	parametersConversion["Rescale Range"] = false;
	parametersConversion["Automatic Input Range"] = true;
	parametersConversion["Use Full OutputRange"] = true;
	dataTypeConversion->run(parametersConversion);

	auto filterScale = iAFilterRegistry::filter("Shift and Scale");
	if (!filterScale)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Shift and Scale' filter, but filter could not be found!")
				.arg(m_fileName));
		return;
	}
	filterScale->addInput(dataTypeConversion->output(0)->itkImage(), "");
	QVariantMap parametersScale;
	parametersScale["Shift"] = m_Parameter["Offset"].toInt();
	parametersScale["Scale"] = m_Parameter["Scale"].toFloat();
	filterScale->run(parametersScale);

	getVtkImageData()->DeepCopy(filterScale->output(0)->vtkImage());
}

void iAIO::readMetaImage( )
{
	loadMetaImageFile(m_fileName);
	postImageReadActions();
}

void iAIO::readSTL( )
{
	auto stlReader = vtkSmartPointer<vtkSTLReader>::New();
	ProgressObserver()->observe(stlReader);
	stlReader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	stlReader->SetOutput(getVtkPolyData());
	stlReader->Update();
	printSTLFileInfos();
	addMsg(tr("File loaded."));
}

bool iAIO::setupVolumeStackMHDReader(QString const & f)
{
	int indexRange[2] = {0, 0};
	int digitsInIndex = 0;
	m_fileNamesBase = f;
	m_extension = "." + QFileInfo(f).suffix();
	iAAttributes params;
	addSeriesParameters(params, m_fileNamesBase, m_extension, digitsInIndex, indexRange);
	iAParameterDlg dlg(m_parent, "Set file parameters", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	auto values = dlg.parameterValues();
	m_fileNamesBase = values[FileNameBase].toString();
	m_extension = values[Extension].toString();
	digitsInIndex = values[NumDigits].toInt();
	indexRange[0] = values[MinimumIndex].toInt();
	indexRange[1] = values[MaximumIndex].toInt();
	fillFileNameArray(indexRange, digitsInIndex);
	return true;
}

QString getParameterValues(QString fileName, QString parameter, int index, QString section = "", QString sep = ":")
{
	if (index < 0 || index>3)
	{
		return 0;
	}
	QString values[4];
	QFile file(fileName);

	if (file.open(QIODevice::ReadOnly))
	{
		QTextStream textStream(&file);
		QString currentLine, currentSection;
		while (!textStream.atEnd())
		{
			currentLine = textStream.readLine();
			if (!currentLine.isEmpty())
			{
				QString currentParameter;
				if ((currentLine.indexOf("[") == 0) && (currentLine.indexOf("[") < currentLine.indexOf("]")))
					currentSection = currentLine.section(" ", 0, 0);
				else
					currentParameter = currentLine.section(" ", 0, 0);

				if ((section != "") && (!currentSection.startsWith(section))) continue;
				else
				{
					if (currentParameter.startsWith(parameter))
					{
						QString temp = currentLine.remove(0, currentLine.indexOf(sep) + 1);
						temp = temp.simplified();
						if (currentParameter == "name")
						{
							values[index] = temp;
							return values[index];
						}

						values[0] = temp.section(" ", 0, 0).trimmed();
						values[1] = temp.section(" ", 1, 1).trimmed();
						values[2] = temp.section(" ", 2, 2).trimmed();

						if (currentLine.indexOf("%") >= 0)
							values[3] = temp.section("%", 1, 1);
					}
				}
			}
		}
		file.close();
	}
	else return "";

	return values[index];
}

bool iAIO::setupVolumeStackVolstackReader( QString const & f )
{
	QFileInfo fi(f);
	m_fileNamesBase		= fi.absolutePath() + "/" + getParameterValues(f, "file_names_base:", 0);
	m_extension			= getParameterValues(f, "extension:", 0);
	int digitsInIndex	= getParameterValues(f, "number_of_digits_in_index:", 0).toInt();
	int indexRange[2]	= {
		getParameterValues(f, "minimum_index:", 0).toInt(),
		getParameterValues(f, "maximum_index:", 0).toInt()};
	m_additionalInfo	= getParameterValues(f, "elementNames:", 0);
	if (m_additionalInfo.isEmpty())
	{
		m_additionalInfo = getParameterValues(f, "energy_range:", 0);
	}

	fillFileNameArray(indexRange, digitsInIndex);
	return true;
}

bool iAIO::setupVolumeStackVolStackWriter(QString const & f)
{
	int numOfDigits = static_cast<int>(std::floor(std::log10(static_cast<double>(m_volumes->size()))) + 1);
	int indexRange[2];
	indexRange[0] = 0;
	indexRange[1] = m_volumes->size()-1;
	m_fileName = f;
	if (!m_fileName.endsWith(VolstackExtension))
	{
		m_fileName.append(VolstackExtension);
	}												// remove .volstack
	m_fileNamesBase = m_fileName.left(m_fileName.length()-VolstackExtension.length());
	m_extension = ".mhd";
	fillFileNameArray(indexRange, numOfDigits);
	return true;
}

void iAIO::fillFileNameArray(int * indexRange, int digitsInIndex, int stepSize)
{
	for (int i=indexRange[0]; i<=indexRange[1]; i += stepSize)
	{
		QString temp = m_fileNamesBase + QString("%1").arg(i, digitsInIndex, 10, QChar('0')) + m_extension;
		m_fileNameArray->InsertNextValue(getLocalEncodingFileName(temp).c_str());
	}
}

bool iAIO::setupVolumeStackReader(QString const & f)
{
	int indexRange[2] = {0, 0};
	int digitsInIndex = 0;
	m_fileNamesBase = f;
	m_extension = "." + QFileInfo(f).suffix();
	iAAttributes params;
	addSeriesParameters(params, m_fileNamesBase, m_extension, digitsInIndex, indexRange);
	auto map = m_rawFileParams.toMap();
	iARawFileParamDlg dlg(f, m_parent, "RAW file specs", params, map, iAMainWindow::get()->brightMode());
	if (!dlg.accepted())
	{
		return false;
	}
	auto values = dlg.parameterValues();
	m_rawFileParams = iARawFileParameters::fromMap(dlg.parameterValues());
	m_fileNamesBase = values[FileNameBase].toString();
	m_extension = values[Extension].toString();
	digitsInIndex = values[NumDigits].toInt();
	indexRange[0] = values[MinimumIndex].toInt();
	indexRange[1] = values[MaximumIndex].toInt();
	fillFileNameArray(indexRange, digitsInIndex);
	return true;
}

bool iAIO::setupRAWReader( QString const & f )
{
	m_fileName = f;
	auto map = m_rawFileParams.toMap();
	iARawFileParamDlg dlg(f, m_parent, "RAW file specs", iAAttributes(), map, iAMainWindow::get()->brightMode());
	if (dlg.accepted())
	{
		m_rawFileParams = iARawFileParameters::fromMap(dlg.parameterValues());
	}
	return dlg.accepted();
}

bool iAIO::setupPARSReader( QString const & f )
{
	m_rawFileParams.m_size[0] = getParameterValues(f,"det_size:", 0).toInt()-1;
	m_rawFileParams.m_size[1] = getParameterValues(f, "det_size:", 1).toInt()-1;
	m_rawFileParams.m_size[2] = getParameterValues(f, "reco_n_proj:", 0).toInt()-1;

	m_rawFileParams.m_spacing[0] = getParameterValues(f, "det_pitch:", 0).toDouble() / (getParameterValues(f, "geo_SD:", 0).toDouble() / getParameterValues(f, "geo_SO:", 0).toDouble());
	m_rawFileParams.m_spacing[1] = getParameterValues(f, "det_pitch:", 1).toDouble() / (getParameterValues(f, "geo_SD:", 0).toDouble() / getParameterValues(f, "geo_SO:", 0).toDouble());
	m_rawFileParams.m_spacing[2] = m_rawFileParams.m_spacing[0] > m_rawFileParams.m_spacing[1] ? m_rawFileParams.m_spacing[1] : m_rawFileParams.m_spacing[0];

	m_rawFileParams.m_scalarType = (getParameterValues(f, "proj_datatype:", 0) == "intensity") ? VTK_UNSIGNED_SHORT: VTK_FLOAT;

	m_fileName = getParameterValues(f,"proj_filename_template_1:",0);
	QFileInfo pars(f);
	if(!QFile::exists(m_fileName))
	{
		if ((m_fileName.lastIndexOf("\\") == -1) && (m_fileName.lastIndexOf("/") == -1))
		{
			m_fileName = pars.canonicalPath() + "/" + m_fileName;
		}
		else if (m_fileName.lastIndexOf("\\") > 0)
		{
			m_fileName = pars.canonicalPath() + "/" + m_fileName.section('\\', -1);
		}
		else if (m_fileName.lastIndexOf("/") > 0)
		{
			m_fileName = pars.canonicalPath() + "/" + m_fileName.section('/', -1);
		}
		else
		{
			m_fileName = QFileDialog::getOpenFileName(m_parent, tr("Specify data File (file path in PARS is wrong)"),
				"", tr("PRO (*.pro);;RAW files (*.raw);;All files (*)"));
		}
	}
	QFile file;
	file.setFileName(m_fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError, QString("PARS reader: Cannot open data file '%1'!").arg(m_fileName));
		return false;
	}
	else
	{
		file.close();
	}
	return true;
}

bool iAIO::setupVGIReader( QString const & f )
{
	m_rawFileParams.m_size[0] = getParameterValues(f,"size", 0, "[file1]", "=").toInt() ;
	m_rawFileParams.m_size[1] = getParameterValues(f,"size", 1, "[file1]", "=").toInt() ;
	m_rawFileParams.m_size[2] = getParameterValues(f,"size", 2, "[file1]", "=").toInt() ;
	if ((m_rawFileParams.m_size[0] == 0) || (m_rawFileParams.m_size[1] == 0) || (m_rawFileParams.m_size[2] == 0))
	{
		m_rawFileParams.m_size[0] = getParameterValues(f,"Size", 0, "[file1]", "=").toInt() ;
		m_rawFileParams.m_size[1] = getParameterValues(f,"Size", 1, "[file1]", "=").toInt() ;
		m_rawFileParams.m_size[2] = getParameterValues(f,"Size", 2, "[file1]", "=").toInt() ;
	}
	if ((m_rawFileParams.m_size[0] == 0) || (m_rawFileParams.m_size[1] == 0) || (m_rawFileParams.m_size[2] == 0))
	{
		LOG(lvlError, "VGI reader: One of the 3 dimensions has size 0!");
		return false;
	}
	m_rawFileParams.m_spacing[0] = getParameterValues(f,"resolution", 0, "[geometry]", "=").toDouble();
	m_rawFileParams.m_spacing[1] = getParameterValues(f,"resolution", 1, "[geometry]", "=").toDouble();
	m_rawFileParams.m_spacing[2] = getParameterValues(f,"resolution", 2, "[geometry]", "=").toDouble();
	if ((m_rawFileParams.m_spacing[0] == 0) || (m_rawFileParams.m_spacing[1] == 0) || (m_rawFileParams.m_spacing[2] == 0))
		m_rawFileParams.m_spacing[0] = m_rawFileParams.m_spacing[1] = m_rawFileParams.m_spacing[2] = 1;
	if ((m_rawFileParams.m_spacing[1] == 0) && (m_rawFileParams.m_spacing[2] == 0))
		m_rawFileParams.m_spacing[1] = m_rawFileParams.m_spacing[2] = m_rawFileParams.m_spacing[0];

	int elementSize = getParameterValues(f,"bitsperelement", 0, "[file1]", "=").toInt();
	if (elementSize == 0) elementSize = getParameterValues(f,"BitsPerElement", 0, "[file1]", "=").toInt();
	if (elementSize == 0)
	{
		LOG(lvlError, "VGI reader: BitsPerElement is 0!");
		return false;
	}

	if (elementSize == 8) m_rawFileParams.m_scalarType = VTK_UNSIGNED_CHAR;
	else if (elementSize == 16) m_rawFileParams.m_scalarType = VTK_UNSIGNED_SHORT;
	else if (elementSize == 32)	m_rawFileParams.m_scalarType = VTK_FLOAT;

	m_rawFileParams.m_headersize = getParameterValues(f,"skipheader", 0, "[file1]", "=").toInt();
	if (m_rawFileParams.m_headersize == 0) m_rawFileParams.m_headersize = getParameterValues(f,"Skipheader", 0, "[file1]", "=").toInt();

	m_fileName = getParameterValues(f,"name",0, "[file1]", "=");
	if (m_fileName == "") m_fileName = getParameterValues(f,"Name",0, "[file1]", "=");
	if (m_fileName == "")
		m_fileName = QFileDialog::getOpenFileName(m_parent, tr("Specify data File (file path in VGI is wrong)"), "", tr("RAW files (*.raw);;REC files (*.rec);;VOL files (*.vol);;All files (*)"));

	QFileInfo pars(f);
	if(!QFile::exists(m_fileName))	{
		if ((m_fileName.lastIndexOf("\\") == -1) && (m_fileName.lastIndexOf("/") == -1))
			m_fileName = pars.canonicalPath() + "/" + m_fileName;
		else if (m_fileName.lastIndexOf("\\") > 0)
			m_fileName = pars.canonicalPath() + "/" + m_fileName.section('\\', -1);
		else if (m_fileName.lastIndexOf("/") > 0)
			m_fileName = pars.canonicalPath() + "/" + m_fileName.section('/', -1);
		else
			m_fileName = QFileDialog::getOpenFileName(m_parent, tr("Specify data File (file path in VGI is wrong)"), "", tr("RAW files (*.raw);;REC files (*.rec);;VOL files (*.vol);;All files (*)"));
	}

	QFile file;
	file.setFileName(m_fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError, QString("VGI reader: Cannot open data file '%1'!").arg(m_fileName));
		return false;
	}
	else file.close();

	return true;
}

bool iAIO::setupNKCReader(QString const& f)
{


	QFile file(f);
	file.open(QFile::ReadOnly | QFile::Text);

	QTextStream in(&file);

	auto text = in.readAll();

	QRegularExpression regexColumns("number of columns : (\\d*)\\D");
	QRegularExpressionMatch matchColumns = regexColumns.match(text);
	if (matchColumns.hasMatch())
	{
		QString columns = matchColumns.captured(1);
		m_rawFileParams.m_size[0] = columns.toInt();
	}

	QRegularExpression regexRows("number of raws : (\\d*)\\D");
	QRegularExpressionMatch matchRows = regexRows.match(text);
	if (matchRows.hasMatch())
	{
		QString rows = matchRows.captured(1);
		m_rawFileParams.m_size[1] = rows.toInt();
	}

	QRegularExpression regexOffset("value offset : (\\d*)\\D");
	QRegularExpressionMatch matchOffset = regexOffset.match(text);
	if (matchOffset.hasMatch())
	{
		QString Offset = matchOffset.captured(1);
		auto value = Offset.toInt();
		m_Parameter.insert("Offset", QVariant(value));
		
	}

	QRegularExpression regexScale("value coefficient : (\\d.\\d*E?-?\\d?)\\D");
	QRegularExpressionMatch matchScale = regexScale.match(text);
	if (matchScale.hasMatch())
	{
		QString scale = matchScale.captured(1);
		auto value = scale.toFloat();
		m_Parameter.insert("Scale", QVariant(value));
	}

	m_rawFileParams.m_size[2] = 1;
	m_rawFileParams.m_scalarType = VTK_TYPE_UINT16;
	m_rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;

	//set Spacing
	m_rawFileParams.m_spacing[0] = 1;
	m_rawFileParams.m_spacing[1] = 1;
	m_rawFileParams.m_spacing[2] = 1;


	m_rawFileParams.m_headersize = file.size() - (m_rawFileParams.m_size[0] * m_rawFileParams.m_size[1] * 2);

	m_fileName = f;

	return true;

}

void iAIO::writeMetaImage( vtkSmartPointer<vtkImageData> imgToWrite, QString fileName )
{
	iAConnector con; con.setImage(imgToWrite); con.modified();
	iAConnector::ITKScalarPixelType itkType = con.itkScalarPixelType();
	iAConnector::ITKPixelType itkPixelType = con.itkPixelType();
	ITK_EXTENDED_TYPED_CALL(write_image_template, itkType, itkPixelType,
		m_compression, fileName, ProgressObserver(), &con);
	addMsg(tr("Saved as file '%1'.").arg(fileName));
}

void iAIO::writeSTL( )
{
	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	ProgressObserver()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	stlWriter->SetInputData(getVtkPolyData());
	stlWriter->SetFileTypeToBinary();
	stlWriter->Write();
	addMsg(tr("Saved as file '%1'.").arg(m_fileName));
	stlWriter->ReleaseDataFlagOn();
}

template <typename T>
void writeImageStack_template(QString const & fileName, iAProgress* p, iAConnector* con, bool comp)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<T, DIM-1> OutputImageType;
	typedef itk::ImageSeriesWriter<InputImageType, OutputImageType> SeriesWriterType;
	auto writer = SeriesWriterType::New();

	typedef itk::NumericSeriesFileNames NameGeneratorType;
	auto nameGenerator = NameGeneratorType::New();

	typename InputImageType::RegionType region = dynamic_cast<InputImageType*>(con->itkImage())->GetLargestPossibleRegion();
	typename InputImageType::IndexType start = region.GetIndex();
	typename InputImageType::SizeType size = region.GetSize();
	nameGenerator->SetStartIndex(start[2]);
	nameGenerator->SetEndIndex(start[2] + size[2] - 1);
	nameGenerator->SetIncrementIndex(1);

	QFileInfo fi(fileName);

	if (fi.completeSuffix().toUpper() == "DCM")	// should be equal to if (ioID == DCM_WRITER)
	{
		auto gdcmIO = itk::GDCMImageIO::New();
		itk::MetaDataDictionary & dict = gdcmIO->GetMetaDataDictionary();
		std::string tagkey, value;
		tagkey = "0008|0060";	//Modality
		value = "CT";			//Computed Tomography (https://wiki.nci.nih.gov/display/CIP/Key+to+two-letter+Modality+Acronyms+in+DICOM)
		itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
		tagkey = "0008|0008";	//Image Type
		value = "ORIGINAL";		//Original image
		itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
		tagkey = "0008|0064";	//Conversion Type
		value = "SI";			//Scanned Image
		itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
		writer->SetImageIO(gdcmIO);
	}
	// to avoid SCIFIO claiming being able to write those image formats and then failing:
	else if (fi.completeSuffix().toUpper() == "BMP")
	{
		auto imgIO = itk::BMPImageIO::New();
		writer->SetImageIO(imgIO);
	}
	else if (fi.completeSuffix().toUpper() == "JPG" || fi.completeSuffix().toUpper() == "JPEG")
	{
		auto imgIO = itk::JPEGImageIO::New();
		writer->SetImageIO(imgIO);
	}
	else if (fi.completeSuffix().toUpper() == "PNG")
	{
		auto imgIO = itk::PNGImageIO::New();
		writer->SetImageIO(imgIO);
	}
	else if (fi.completeSuffix().toUpper() == "TIF" || fi.completeSuffix().toUpper() == "TIFF")
	{
		auto imgIO = itk::TIFFImageIO::New();
		writer->SetImageIO(imgIO);
	}

	QString length = QString::number(size[2]);
	QString format(fi.absolutePath() + "/" + fi.baseName() + "%0" + QString::number(length.size()) + "d." + fi.completeSuffix());
	nameGenerator->SetSeriesFormat( getLocalEncodingFileName(format).c_str());
	writer->SetFileNames(nameGenerator->GetFileNames());
	writer->SetInput(dynamic_cast< InputImageType * > (con->itkImage()));
	writer->SetUseCompression(comp);
	p->observe(writer);
	writer->Update();
}

void iAIO::writeImageStack( )
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	getConnector()->setImage(getVtkImageData());
	const ScalarPixelType pixelType = getConnector()->itkScalarPixelType();
	const PixelType imagePixelType = getConnector()->itkPixelType();
	ITK_EXTENDED_TYPED_CALL(writeImageStack_template, pixelType, imagePixelType,
		m_fileName, ProgressObserver(), getConnector(), false);  //compression Hard coded to false, because the used m_compression was used for stl
	addMsg(tr("%1 Image Stack saved; base file name: %2")
		.arg(QFileInfo(m_fileName).completeSuffix().toUpper())
		.arg(m_fileName));
}

//****************************************************
//*                                                  *
//*                  2D reading                      *
//*                                                  *
//****************************************************

bool iAIO::setupStackReader( QString const & f )
{
	int indexRange[2];
	int digits;
	determineStackParameters(f, m_fileNamesBase, m_extension, indexRange, digits);
	iAAttributes params;
	addSeriesParameters(params, m_fileNamesBase, m_extension, digits, indexRange);
	addAttr(params, "Step", iAValueType::Discrete, 1);
	addAttr(params, "Spacing X", iAValueType::Continuous, m_rawFileParams.m_spacing[0]);
	addAttr(params, "Spacing Y", iAValueType::Continuous, m_rawFileParams.m_spacing[1]);
	addAttr(params, "Spacing Z", iAValueType::Continuous, m_rawFileParams.m_spacing[2]);
	addAttr(params, "Origin X", iAValueType::Continuous, m_rawFileParams.m_origin[0]);
	addAttr(params, "Origin Y", iAValueType::Continuous, m_rawFileParams.m_origin[1]);
	addAttr(params, "Origin Z", iAValueType::Continuous, m_rawFileParams.m_origin[2]);
	iAParameterDlg dlg(m_parent, "Set file parameters", params, "Please check these automatically determined settings:");
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	auto values = dlg.parameterValues();
	m_fileNamesBase = values[FileNameBase].toString();
	m_extension = values[Extension].toString();
	digits = values[NumDigits].toInt();
	indexRange[0] = values[MinimumIndex].toInt();
	indexRange[1] = values[MaximumIndex].toInt();
	int stepSize = values["Step"].toInt();
	m_rawFileParams.m_spacing[0] = values["Spacing X"].toDouble();
	m_rawFileParams.m_spacing[1] = values["Spacing Y"].toDouble();
	m_rawFileParams.m_spacing[2] = values["Spacing Z"].toDouble();
	m_rawFileParams.m_origin[0] = values["Origin X"].toDouble();
	m_rawFileParams.m_origin[1] = values["Origin Y"].toDouble();
	m_rawFileParams.m_origin[2] = values["Origin Z"].toDouble();
	fillFileNameArray(indexRange, digits, stepSize);
	return true;
}

void iAIO::readImageStack()
{
	vtkSmartPointer<vtkImageReader2> imgReader;
	switch (m_ioID)
	{
		case TIF_STACK_READER: imgReader = vtkSmartPointer<vtkTIFFReader>::New(); break;
		case JPG_STACK_READER: imgReader = vtkSmartPointer<vtkJPEGReader>::New(); break;
		case PNG_STACK_READER: imgReader = vtkSmartPointer<vtkPNGReader>::New(); break;
		case BMP_STACK_READER: imgReader = vtkSmartPointer<vtkBMPReader>::New(); break;
		default: throw std::runtime_error("Invalid Image Stack IO id, aborting.");
	}
	imgReader->ReleaseDataFlagOn();
	ProgressObserver()->observe(imgReader);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	imgReader->SetFileNames(m_fileNameArray);
	imgReader->SetDataOrigin(m_rawFileParams.m_origin);
	imgReader->SetDataSpacing(m_rawFileParams.m_spacing);
	imgReader->SetOutput(getVtkImageData());
	imgReader->Update();
	addMsg(tr("Loading image stack completed."));
}

void iAIO::storeIOSettings()
{
	QSettings settings;
	settings.setValue("IO/rawSizeX", m_rawFileParams.m_size[0]);
	settings.setValue("IO/rawSizeY", m_rawFileParams.m_size[1]);
	settings.setValue("IO/rawSizeZ", m_rawFileParams.m_size[2]);
	settings.setValue("IO/rawSpaceX", m_rawFileParams.m_spacing[0]);
	settings.setValue("IO/rawSpaceY", m_rawFileParams.m_spacing[1]);
	settings.setValue("IO/rawSpaceZ", m_rawFileParams.m_spacing[2]);
	settings.setValue("IO/rawOriginX", m_rawFileParams.m_origin[0]);
	settings.setValue("IO/rawOriginY", m_rawFileParams.m_origin[1]);
	settings.setValue("IO/rawOriginZ", m_rawFileParams.m_origin[2]);
	settings.setValue("IO/rawScalar", m_rawFileParams.m_scalarType);
	settings.setValue("IO/rawByte", m_rawFileParams.m_byteOrder);
	settings.setValue("IO/rawHeader", m_rawFileParams.m_headersize);
}

void iAIO::loadIOSettings()
{
	QSettings settings;
	iARawFileParameters defaultRawParams;
	m_rawFileParams.m_origin[0] = settings.value("IO/rawOriginX", defaultRawParams.m_origin[0]).toDouble();
	m_rawFileParams.m_origin[1] = settings.value("IO/rawOriginY", defaultRawParams.m_origin[1]).toDouble();
	m_rawFileParams.m_origin[2] = settings.value("IO/rawOriginZ", defaultRawParams.m_origin[2]).toDouble();
	m_rawFileParams.m_spacing[0] = settings.value("IO/rawSpaceX", defaultRawParams.m_spacing[0]).toDouble();
	if (m_rawFileParams.m_spacing[0] == 0) m_rawFileParams.m_spacing[0] = 1;
	m_rawFileParams.m_spacing[1] = settings.value("IO/rawSpaceY", defaultRawParams.m_spacing[1]).toDouble();
	if (m_rawFileParams.m_spacing[1] == 0) m_rawFileParams.m_spacing[1] = 1;
	m_rawFileParams.m_spacing[2] = settings.value("IO/rawSpaceZ", defaultRawParams.m_spacing[2]).toDouble();
	if (m_rawFileParams.m_spacing[2] == 0) m_rawFileParams.m_spacing[2] = 1;
	m_rawFileParams.m_size[0] = settings.value("IO/rawSizeX", defaultRawParams.m_size[0]).toInt();
	m_rawFileParams.m_size[1] = settings.value("IO/rawSizeY", defaultRawParams.m_size[1]).toInt();
	m_rawFileParams.m_size[2] = settings.value("IO/rawSizeZ", defaultRawParams.m_size[2]).toInt();
	m_rawFileParams.m_scalarType = settings.value("IO/rawScalar", defaultRawParams.m_scalarType).toInt();
	m_rawFileParams.m_byteOrder = settings.value("IO/rawByte", defaultRawParams.m_byteOrder).toInt();
	m_rawFileParams.m_headersize = settings.value("IO/rawHeader", defaultRawParams.m_headersize).toInt();
}

void iAIO::printSTLFileInfos()
{
	// TODO: show this information in img properties instead of log
	addMsg(tr("  Cells: %1").arg(getVtkPolyData()->GetNumberOfCells()));
	addMsg(tr("  Points: %1").arg(getVtkPolyData()->GetNumberOfPoints()));
	addMsg(tr("  Polygons: %1").arg(getVtkPolyData()->GetNumberOfPolys()));
	addMsg(tr("  Lines: %1").arg(getVtkPolyData()->GetNumberOfLines()));
	addMsg(tr("  Strips: %1").arg(getVtkPolyData()->GetNumberOfStrips()));
	addMsg(tr("  Pieces: %1").arg(getVtkPolyData()->GetNumberOfPieces()));
}

QString const & iAIO::additionalInfo()
{
	return m_additionalInfo;
}

void iAIO::setAdditionalInfo(QString const & additionalInfo)
{
	m_additionalInfo = additionalInfo;
}

QString const & iAIO::fileName()
{
	return m_fileName;
}

QSharedPointer<iAModalityList> iAIO::modalities()
{
	return m_modalities;
}

int iAIO::ioID() const
{
	return m_ioID;
}
