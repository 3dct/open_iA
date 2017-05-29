/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iAIO.h"

#include "defines.h"
#include "dlg_commoninput.h"
#include "dlg_openfile_sizecheck.h"
#include "iAAmiraMeshIO.h"
#include "iAConnector.h"
#include "iAExceptionThrowingErrorObserver.h"
#include "iAExtendedTypedCallHelper.h"
#include "iAObserverProgress.h"
#include "iAOIFReader.h"
#include "iAProgress.h"
#include "iAVolumeStack.h"
#include "iAToolsVTK.h"

#include <itkExceptionObject.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOBase.h>
#include <itkImageSeriesReader.h>
#include <itkImageSeriesWriter.h>
#include <itkNrrdImageIO.h>
#include <itkNumericSeriesFileNames.h>
#include <itkRawImageIO.h>

#include <vtkBMPReader.h>
#include <vtkImageData.h>
#include <vtkJPEGReader.h>
#include <vtkPNGReader.h>
#include <vtkPolyData.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTIFFReader.h>
#include <vtkVersion.h>
#include <vtkXMLImageDataReader.h>

#include <QDateTime>
#include <QFileDialog>
#include <QSettings>
#include <QStringList>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


const QString iAIO::VolstackExtension(".volstack");


/**
 * \param	d		dim.
 * \param	h		headersize.
 * \param	bO		byteorder.
 * \param	e		extent.
 * \param	sp		spacing.
 * \param	o		origin.
  * \param	s		filename.
 * \param	p		progress information.
 * \param	image	Input image.
 * \param			The.
 * \return	int Status-Code.
 */
template<class T> int read_raw_image_template ( int d, unsigned long h, int bO,
											  int* e, double* sp, double* o, QString f, 
											  iAProgress* p, iAConnector* image, T  )
{
	typedef itk::RawImageIO<T, DIM> RawImageIOType;
	typename RawImageIOType::Pointer io = RawImageIOType::New();
	
	io->SetFileName(f.toLatin1().data());
	io->SetHeaderSize(h);
	for(int i=0; i<DIM; i++)
	{
		io->SetDimensions(i, e[2*i+1] + 1);
		io->SetSpacing(i, sp[i]);
		io->SetOrigin(i, o[i]);
	}
	
	if (bO == VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN)
		io->SetByteOrderToLittleEndian();
	else
		io->SetByteOrderToBigEndian();
	
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();
	
	reader->SetFileName(f.toLatin1().data());
	reader->SetImageIO(io);
	p->Observe( reader );
	
	reader->Modified();
	reader->Update();
	
	image->SetImage(reader->GetOutput());
	image->Modified();
	
	reader->ReleaseDataFlagOn();
	
	return EXIT_SUCCESS;
}


/**
 * \param	s		filename.
 * \param	p		progress information.
 * \param	image	Input image.
 * \param			The.
 * \return	int Status-Code.
 */
template<class T> 
int read_image_template( QString f, iAProgress* p, iAConnector* image  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();
	
	reader->SetFileName(f.toLatin1().data());
	
	p->Observe( reader );
	
	reader->Update();
	
	image->SetImage(reader->GetOutput());
	image->Modified();
	
	reader->ReleaseDataFlagOn();
	
	return EXIT_SUCCESS;
}


/**
 * \param	comp	If to use compression.
 * \param	f		filename.
 * \param	p		progress information.
 * \param	image	Input image.
 * \param			The datatype.
 * \return	int Status-Code.
 */
template<class T> 
int write_image_template(  bool comp, QString f, iAProgress* p, iAConnector* image  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileWriter<InputImageType> WriterType;
	typename WriterType::Pointer writer = WriterType::New();
	
	writer->SetFileName(f.toLatin1().data());
	writer->SetInput( dynamic_cast< InputImageType * > ( image->GetITKImage() ) );
	writer->SetUseCompression(comp);
	p->Observe( writer );
	writer->Update();

	writer->ReleaseDataFlagOn();
	
	return EXIT_SUCCESS;
}


iAIO::iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *par, vector<vtkSmartPointer<vtkImageData> > * volumes, vector<QString> * fileNames)
	: iAAlgorithm( i, p, logger, par ),
	m_volumes(volumes),
	m_fileNames_volstack(fileNames)
{
	init(par);
}


iAIO::iAIO( iALogger* logger, QObject *parent /*= 0*/, vector<vtkSmartPointer<vtkImageData> > * volumes /*= 0*/, vector<QString> * fileNames /*= 0*/ )
	: iAAlgorithm( 0, 0, logger, parent),
	m_volumes(volumes),
	m_fileNames_volstack(fileNames)
{
	init(parent);
}


void iAIO::init(QObject *par)
{
	parent = (QWidget*)par;

	fileName = "";
	fileNameArray = vtkStringArray::New();
	extent[0] = 0; extent[1] = 1; extent[2] = 0; extent[3] = 1; extent[4] = 0, extent[5] = 1;
	spacing[0] = 1.0; spacing[1] = 1.0; spacing[2] = 1.0;
	origin[0] = 0.0; origin[1] = 0.0; origin[2] = 0.0;
	dim = 3;
	headersize = 0;
	scalarType = 0;
	byteOrder = 1;

	ioID = 0;
	iosettingsreader();

	stlReader = vtkSTLReader::New();
	observerProgress = iAObserverProgress::New();

	if (par)
	{
		connect(this, SIGNAL(msg(QString)), par, SLOT(addMsg(QString)));
	}
}


iAIO::~iAIO()
{
	fileNameArray->Delete();
	stlReader->ReleaseDataFlagOn();
	stlReader->Delete();
	if (observerProgress) observerProgress->Delete();
}


void iAIO::run()
{
	qApp->processEvents();
	bool rv = false;
	
	// todo: hook for plugins!
	switch (ioID)
	{
		case MHD_WRITER:
			rv = writeMetaImage(); break;
		case VOLUME_STACK_VOLSTACK_WRITER:
			rv = writeVolumeStack(); break;
		case STL_WRITER:
			rv = writeSTL(); break;
		case TIF_STACK_WRITER:
		case JPG_STACK_WRITER:
		case PNG_STACK_WRITER:
		case BMP_STACK_WRITER:
		case DCM_WRITER:
			rv = writeImageStack(); break;
		case MHD_READER:
			rv = readMetaImage(); break;
		case STL_READER:
			rv = readSTL(); break;
		case RAW_READER:
		case PARS_READER:
		case VGI_READER:
			rv = readImageData(); break;
		case VOLUME_STACK_READER:
			rv = readVolumeStack(); break;
		case VOLUME_STACK_MHD_READER:
		case VOLUME_STACK_VOLSTACK_READER:
			rv = readVolumeMHDStack(); break;
		case TIF_STACK_READER:
		case JPG_STACK_READER:
		case PNG_STACK_READER:
		case BMP_STACK_READER:
			rv = readImageStack(); break;
		case DCM_READER:
			rv = readDCM(); break;
		case NRRD_READER:
			rv = readNRRD(); break;
		case OIF_READER: {
			IO::OIF::Reader r;
			r.read(getFileName(), getConnector(), m_channel, m_volumes);
			//if (!m_volumes)
			{
				postImageReadActions();
			}
			rv = true;
			break;
		}
		case AM_READER: {
			vtkSmartPointer<vtkImageData> img = iAAmiraMeshIO::Load(getFileName());
			if (!img)
			{
				break;
			}
			getConnector()->SetImage(img);
			getConnector()->Modified();
			postImageReadActions();
			rv = true;
			break;
		}
		case AM_WRITER: {
			rv = iAAmiraMeshIO::Write(getFileName(), getVtkImageData());
			break;
		}
		case CSV_WRITER: {
			// TODO: write more than one modality!
			auto img = getVtkImageData();
			int numberOfComponents = img->GetNumberOfScalarComponents();
			std::ofstream out(getFileName().toStdString());
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
		case VTI_READER: {
			vtkSmartPointer<vtkXMLImageDataReader> reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
			reader->SetFileName(getFileName().toStdString().c_str());
			reader->Update();
			getConnector()->SetImage(reader->GetOutput());
			getConnector()->Modified();
			postImageReadActions();
			rv = true;
			break;
		}
		case UNKNOWN_READER:
		default:
			emit msg(tr("  unknown reader type"));
	}

	if (rv) {
		//emit msg(tr("   File I/O successful!"));
		if ((ioID == MHD_WRITER) || (ioID == STL_WRITER) || (ioID == TIF_STACK_WRITER) 
			|| (ioID == JPG_STACK_WRITER) || (ioID == PNG_STACK_WRITER)|| (ioID == BMP_STACK_WRITER) || (ioID ==DCM_WRITER) )
			emit done(true);
		else emit done();
	} else {
		emit msg(tr("   FILE I/O FAILED!"));
		emit failed();
	}
}


/**
 * \return	true if it succeeds, false if it fails. 
 */
bool iAIO::setupIO( IOType type, QString f, bool c, int channel)
{
	ioID = type;
	m_channel = channel;

	f_dir = QFileInfo(f).absoluteDir();	
	
	// TODO: hook for plugins!
	switch (ioID)
	{
		case MHD_WRITER:
		case STL_WRITER:
		case MHD_READER:
		case STL_READER:
			fileName = f; compression = c; break;
		case RAW_READER:
			return setupRAWReader(f);
		case PARS_READER:
			return setupPARSReader(f);
		case VGI_READER:
			return setupVGIReader(f);
		case TIF_STACK_READER:
		case JPG_STACK_READER:
		case PNG_STACK_READER:
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

		case TIF_STACK_WRITER:  // intentional fall-throughs
		case JPG_STACK_WRITER:
		case PNG_STACK_WRITER:
		case BMP_STACK_WRITER:
		case DCM_READER:
		case DCM_WRITER:
		case NRRD_READER:
		case OIF_READER:
		case AM_READER:
		case AM_WRITER:
		case CSV_WRITER:
		case VTI_READER:
			fileName = f; break;

		case UNKNOWN_READER: 
		default:
			emit msg(tr("  unknown IO type")); 
			return false;
	}
	return true;
}


/**
 * \return	true if successful
 */
bool iAIO::readNRRD(){

	try	{
		typedef itk::Vector<signed short, 2>	VectorType; 
		typedef itk::Image<VectorType, DIM>		DiffusionImageType; 
		typedef DiffusionImageType::Pointer		DiffusionImagePointer; 

		typedef itk::ImageFileReader<DiffusionImageType> FileReaderType; 
		FileReaderType::Pointer reader = FileReaderType::New();

		reader->SetFileName(fileName.toStdString()); 
		reader->Update();

		itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
		io->SetFileType( itk::ImageIOBase::ASCII);

		iAConnector *image = getConnector(); 
		image->SetImage(reader->GetOutput());
		image->Modified();
		postImageReadActions();
		iosettingswriter();
	}
	catch (itk::ExceptionObject &ex)
	{
		emit msg("Error occurred reading NRRD images: " + QString(ex.what()));
		return false;
	}
	return true;
}


/**
 * \brief	This function reads a series of DICOM images.
 */
bool iAIO::readDCM()
{
	typedef signed short PixelType; 

	typedef itk::Image<PixelType, DIM> ImageType; 
	typedef itk::ImageSeriesReader<ImageType> ReaderType; 
	ReaderType::Pointer reader = ReaderType::New();

	typedef itk::GDCMImageIO ImageIOType; 
	ImageIOType::Pointer dicomIO = ImageIOType::New();	
	reader->SetImageIO( dicomIO );

	typedef itk::GDCMSeriesFileNames NamesGeneratorType;
	NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();

	nameGenerator->SetUseSeriesDetails(true);
	nameGenerator->SetDirectory(f_dir.canonicalPath().toStdString());

	try	{
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

		try	{
			reader->Update();
		}
		catch (itk::ExceptionObject &ex)
		{
			emit msg("Error occurred reading Dicom series(reader->Update()): " + QString(ex.what()));
			return false;
		}

		reader->Modified();
		reader->Update();

		iAConnector *image = getConnector(); 
		image->SetImage(reader->GetOutput());
		image->Modified();
		postImageReadActions();
		iosettingswriter();

	}
	catch (itk::ExceptionObject &ex)
	{
		emit msg("Error occurred reading Dicom series: " + QString(ex.what()));
		return false;
	}
	return true;
}


bool iAIO::loadMetaImageFile(QString const & fileName)
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	itk::ImageIOBase::Pointer imageIO;
	QString errorMsg;
	try
	{
		imageIO =
			itk::ImageIOFactory::CreateImageIO(fileName.toLatin1(), itk::ImageIOFactory::ReadMode);
	} catch(itk::ExceptionObject& e)
	{
		imageIO = 0;
		errorMsg = e.what();
	}

	if (!imageIO)
	{
		emit msg(tr("  Could not open file %1, aborting loading (error message: %2).").arg(fileName).arg(errorMsg));
		return false;
	}

	try
	{
		imageIO->SetFileName(fileName.toLatin1());
		imageIO->ReadImageInformation();
		const ScalarPixelType pixelType = imageIO->GetComponentType();
		const PixelType imagePixelType = imageIO->GetPixelType();
		ITK_EXTENDED_TYPED_CALL(read_image_template, pixelType, imagePixelType,
			fileName, getItkProgress(), getConnector());
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return false;
	}
	return true;
}


bool iAIO::readVolumeMHDStack()
{
	if (fileNameArray->GetSize() == 0)
	{
		emit msg(tr("  No files matched the given criteria!"));
		return false;
	}
	for (int m=0; m<=fileNameArray->GetMaxId(); m++)
	{
		fileName=(fileNameArray->GetValue(m));
		if (!loadMetaImageFile(fileName))
		{
			return false;
		}
		if(m_volumes)
		{
			vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
			image->DeepCopy(getConnector()->GetVTKImage());
			m_volumes->push_back(image);
		}
		if(m_fileNames_volstack)
			m_fileNames_volstack->push_back(fileName);

		int progress = (fileNameArray->GetMaxId() == 0) ? 100 : (m * 100) / fileNameArray->GetMaxId();
		observerProgress->manualProgressUpdate(progress);
	}

	emit msg(tr("%1  Loading sequence completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: "+ fileName );
	if(getVtkImageData()) 
		printFileInfos();

	iosettingswriter();

	return true;
}


bool iAIO::readVolumeStack()
{
	for (int m=0; m<=fileNameArray->GetMaxId(); m++)
	{
		fileName=(fileNameArray->GetValue(m));
		if (!readRawImage())
		{
			return false;
		}

		vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
		image->DeepCopy(getConnector()->GetVTKImage());
		if(m_volumes)
			m_volumes->push_back(image);
		if(m_fileNames_volstack)
			m_fileNames_volstack->push_back(fileName);

		int progress = (m * 100) / fileNameArray->GetMaxId();
		observerProgress->manualProgressUpdate(progress);
	}

	emit msg(tr("%1  Loading sequence completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: "+ fileName );
	printFileInfos();
	iosettingswriter();

	return true;
}


bool iAIO::writeVolumeStack()
{
	// write .volstack file:
	QFile volstackFile(fileName);
	QFileInfo fi(fileName);
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
	for (int m=0; m <= fileNameArray->GetMaxId(); m++)
	{
		fileName = (fileNameArray->GetValue(m));
		setImageData(m_volumes->at(m).GetPointer());
		writeMetaImage();
		int progress = (m * 100) / fileNameArray->GetMaxId();
		observerProgress->manualProgressUpdate(progress);
	}
	return true;
}


bool iAIO::readRawImage()
{
	try
	{
		switch (scalarType) // This filter handles all types
		{
		case VTK_UNSIGNED_CHAR:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<unsigned char>(0));  break;
		case VTK_CHAR:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<char>(0));  break;
		case VTK_UNSIGNED_SHORT:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<unsigned short>(0));  break;
		case VTK_SHORT:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<short>(0));  break;
		case VTK_UNSIGNED_INT:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<unsigned int>(0));  break;
		case VTK_INT:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<int>(0));  break;
		case VTK_UNSIGNED_LONG:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<unsigned long>(0));  break;
		case VTK_LONG:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<int>(0));  break;
		case VTK_FLOAT:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<float>(0));  break;
		case VTK_DOUBLE:
			read_raw_image_template(dim, headersize, byteOrder, extent, spacing, origin, fileName, getItkProgress(), getConnector(), static_cast<double>(0));  break;
		case VTK_VOID:
		default:
			emit msg(tr("  unknown component type"));
			return false;
		}
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return false;
	}
	return true;
}


void iAIO::postImageReadActions()
{
	getVtkImageData()->ReleaseData();
	getVtkImageData()->Initialize();
	getVtkImageData()->DeepCopy(getConnector()->GetVTKImage());
	getVtkImageData()->CopyInformationFromPipeline(getConnector()->GetVTKImage()->GetInformation());

	emit msg(tr("%1  Loading sequence completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: " + fileName);
	printFileInfos();
}


bool iAIO::readImageData()
{
	if (!readRawImage())
	{
		return false;
	}
	postImageReadActions();

	iosettingswriter();
	
	return true;
}


bool iAIO::readMetaImage( )
{
	if (!loadMetaImageFile(fileName))
	{
		return false;
	}
	postImageReadActions();
	
	return true;
}


bool iAIO::readSTL( )
{
	stlReader->AddObserver(vtkCommand::ProgressEvent, observerProgress);
	stlReader->SetFileName(fileName.toLatin1());
	stlReader->SetOutput(getVtkPolyData());
	stlReader->Update();

	printSTLFileInfos();
	
	emit msg(tr("%1  Loading sequence completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: "+ fileName );
	
	return true;
}


bool iAIO::setupVolumeStackMHDReader(QString f)
{
	int indexRange[2] = {0, 0};
	int digitsInIndex = 0;

	fileNamesBase = f;
	extension = "." + QFileInfo(f).suffix();
	QStringList inList		= (QStringList() 
		<< tr("#File Names Base") << tr("#Extension")
		<< tr("#Number of Digits in Index")
		<< tr("#Minimum Index")  << tr("#Maximum Index") );
	QList<QVariant> inPara	= (QList<QVariant>() 
		<< fileNamesBase << extension 
		<< tr("%1").arg(digitsInIndex) 
		<< tr("%1").arg(indexRange[0]) << tr("%1").arg(indexRange[1]) );

	dlg_commoninput *dlg = new dlg_commoninput (parent, "Set file parameters", 5, inList, inPara, NULL);

	if (dlg->exec() == QDialog::Accepted){

		fileNamesBase = dlg->getText()[0];
		extension = dlg->getText()[1];
		digitsInIndex = dlg->getValues()[2];
		indexRange[0] = dlg->getValues()[3]; indexRange[1]= dlg->getValues()[4];
	
		FillFileNameArray(indexRange, digitsInIndex);
	}
	else return false;

	return true;
}


QString getParameterValues(QString fileName, QString parameter, int index, QString section = "", QString sep = ":")
{
	if (index<0 || index>3)
		return 0;
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


bool iAIO::setupVolumeStackVolstackReader( QString f )
{
	QFileInfo fi(f);
	fileNamesBase		= fi.absolutePath() + "/" + getParameterValues(f, "file_names_base:", 0);
	extension			= getParameterValues(f, "extension:", 0);
	int digitsInIndex	= getParameterValues(f, "number_of_digits_in_index:", 0).toInt();
	int indexRange[2]	= {
		getParameterValues(f, "minimum_index:", 0).toInt(),
		getParameterValues(f, "maximum_index:", 0).toInt()};
	m_additionalInfo	= getParameterValues(f, "elementNames:", 0);
	if (m_additionalInfo.isEmpty())
	{
		m_additionalInfo = getParameterValues(f, "energy_range:", 0);
	}

	FillFileNameArray(indexRange, digitsInIndex);
	return true;
}


bool iAIO::setupVolumeStackVolStackWriter(QString f)
{
	int numOfDigits = static_cast<int>(std::floor(std::log10(static_cast<double>(m_volumes->size()))) + 1);
	int indexRange[2];
	indexRange[0] = 0;
	indexRange[1] = m_volumes->size()-1;
	fileName = f;
	if (!fileName.endsWith(VolstackExtension))
	{
		fileName.append(VolstackExtension);
	}												// remove .volstack
	fileNamesBase = fileName.left(fileName.length()-VolstackExtension.length());
	extension = ".mhd";
	FillFileNameArray(indexRange, numOfDigits);
	return true;
}


void iAIO::FillFileNameArray(int * indexRange, int digitsInIndex)
{
	for (int i=indexRange[0]; i<=indexRange[1]; i++)
	{
		QString temp = fileNamesBase + QString("%1").arg(i, digitsInIndex, 10, QChar('0')) + extension;
		fileNameArray->InsertNextValue(temp.toLatin1());
	}
}

unsigned int mapVTKByteOrderToIdx(unsigned int vtkByteOrder)
{
	switch (vtkByteOrder)
	{
	default:
	case VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN: return 0;
	case VTK_FILE_BYTE_ORDER_BIG_ENDIAN: return 1;
	}
}

unsigned int mapVTKTypeToIdx(unsigned int vtkScalarType)
{
	switch (vtkScalarType)
	{
		case VTK_UNSIGNED_CHAR: return 0;
		case VTK_CHAR: return 1;
		default:
		case VTK_UNSIGNED_SHORT: return 2;
		case VTK_SHORT: return 3;
		case VTK_UNSIGNED_INT: return 4;
		case VTK_INT: return 5;
		case VTK_FLOAT: return 6;
		case VTK_DOUBLE: return 7;
	}
}


bool iAIO::setupVolumeStackReader(QString f)
{
	int indexRange[2] = {0, 0};
	int digitsInIndex = 0;
	spacing[0]=0;
	spacing[1]=0;
	spacing[2]=0;

	fileNamesBase = f;
	extension = "." + QFileInfo(f).suffix();
	QStringList datatype = (QStringList()
		<< tr("VTK_UNSIGNED_CHAR") << tr("VTK_CHAR")
		<< tr("VTK_UNSIGNED_SHORT") << tr("VTK_SHORT")
		<< tr("VTK_UNSIGNED_INT") << tr("VTK_INT")
		<< tr("VTK_FLOAT") << tr("VTK_DOUBLE"));
	datatype[mapVTKTypeToIdx(rawScalarType)] = "!" + datatype[mapVTKTypeToIdx(rawScalarType)];
	QStringList byteOrderStr = (QStringList() << tr("Little Endian") << tr("Big Endian"));
	byteOrderStr[mapVTKByteOrderToIdx(rawByteOrder)] = "!" + byteOrderStr[mapVTKByteOrderToIdx(rawByteOrder)];
	QStringList inList		= (QStringList() 
		<< tr("#File Names Base") << tr("#Extension") 
		<< tr("#Number of Digits in Index")
		<< tr("#Minimum Index")  << tr("#Maximum Index")
		<< tr("#Size X") << tr("#Size Y") << tr("#Size Z")
		<< tr("#Spacing X") << tr("#Spacing Y") << tr("#Spacing Z")
		<< tr("#Origin X") << tr("#Origin Y") << tr("#Origin Z")
		<< tr("#Headersize")
		<< tr("+Data Type")
		<< tr("+Byte Order") );
	QList<QVariant> inPara	= (QList<QVariant>() 
		<< fileNamesBase << extension 
		<< tr("%1").arg(digitsInIndex) 
		<< tr("%1").arg(indexRange[0]) << tr("%1").arg(indexRange[1])
		<< tr("%1").arg(rawSizeX) << tr("%1").arg(rawSizeY) << tr("%1").arg(rawSizeZ)
		<< tr("%1").arg(spacing[0]) << tr("%1").arg(spacing[1]) << tr("%1").arg(spacing[2])
		<< tr("%1").arg(origin[0]) << tr("%1").arg(origin[1]) << tr("%1").arg(origin[2])
		<< tr("%1").arg(rawHeaderSize)
		<< datatype
		<< byteOrderStr);

	dlg_openfile_sizecheck *dlg = new dlg_openfile_sizecheck (true, parent, "RAW file specs", 17, inList, inPara, NULL, f);
	
	if (dlg->exec() == QDialog::Accepted){

		rawSizeX = dlg->getValues()[5]; rawSizeY = dlg->getValues()[6]; rawSizeZ = dlg->getValues()[7];      
		extent[0] = 0; extent[2] = 0; extent[4] = 0;
		extent[1] = rawSizeX; extent[3]= rawSizeY; extent[5] = rawSizeZ;      
		extent[1]--; extent[3]--; extent[5]--;

		fileNamesBase = dlg->getText()[0];
		extension = dlg->getText()[1];
		digitsInIndex = dlg->getValues()[2];
		indexRange[0] = dlg->getValues()[3]; indexRange[1]= dlg->getValues()[4];
		spacing[0] = dlg->getValues()[8]; spacing[1]= dlg->getValues()[9]; spacing[2] = dlg->getValues()[10];
		origin[0] = dlg->getValues()[11]; origin[1]= dlg->getValues()[12]; origin[2] = dlg->getValues()[13];

		rawHeaderSize = dlg->getValues()[15];
		headersize = rawHeaderSize;
		 
		if (dlg->getComboBoxValues()[14] == "VTK_UNSIGNED_CHAR") scalarType = VTK_UNSIGNED_CHAR;
		if (dlg->getComboBoxValues()[14] == "VTK_CHAR") scalarType = VTK_CHAR;
		if (dlg->getComboBoxValues()[14] == "VTK_UNSIGNED_SHORT") scalarType = VTK_UNSIGNED_SHORT;
		if (dlg->getComboBoxValues()[14] == "VTK_SHORT") scalarType = VTK_SHORT;
		if (dlg->getComboBoxValues()[14] == "VTK_UNSIGNED_INT") scalarType = VTK_UNSIGNED_INT;
		if (dlg->getComboBoxValues()[14] == "VTK_INT") scalarType = VTK_INT;
		if (dlg->getComboBoxValues()[14] == "VTK_FLOAT") scalarType = VTK_FLOAT;
		if (dlg->getComboBoxValues()[14] == "VTK_DOUBLE") scalarType = VTK_DOUBLE;
		rawScalarType = scalarType;

		if (dlg->getComboBoxValues()[16] == "Little Endian") 
		byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
		else if (dlg->getComboBoxValues()[16] == "Big Endian") 
		byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;

		rawByteOrder = byteOrder;

		FillFileNameArray(indexRange, digitsInIndex);
	}
	else return false;

	return true;
}


bool iAIO::setupRAWReader( QString f )
{
	QStringList datatype = (QStringList()
		<< tr("VTK_UNSIGNED_CHAR") << tr("VTK_CHAR")
		<< tr("VTK_UNSIGNED_SHORT") << tr("VTK_SHORT")
		<< tr("VTK_UNSIGNED_INT") << tr("VTK_INT")
		<< tr("VTK_FLOAT") << tr("VTK_DOUBLE"));
	datatype[mapVTKTypeToIdx(rawScalarType)] = "!" + datatype[mapVTKTypeToIdx(rawScalarType)];
	QStringList byteOrderStr = (QStringList() << tr("Little Endian") << tr("Big Endian"));
	byteOrderStr[mapVTKByteOrderToIdx(rawByteOrder)] = "!" + byteOrderStr[mapVTKByteOrderToIdx(rawByteOrder)];
	QStringList inList		= (QStringList() 
		<< tr("#Size X") << tr("#Size Y") << tr("#Size Z")
		<< tr("#Spacing X") << tr("#Spacing Y") << tr("#Spacing Z")
		<< tr("#Origin X") << tr("#Origin Y") << tr("#Origin Z")
		<< tr("#Headersize")
		<< tr("+Data Type")
		<< tr("+Byte Order") );

	QList<QVariant> inPara	= (QList<QVariant>() 
		<< tr("%1").arg(rawSizeX) << tr("%1").arg(rawSizeY) << tr("%1").arg(rawSizeZ)
		<< tr("%1").arg(rawSpaceX) << tr("%1").arg(rawSpaceY) << tr("%1").arg(rawSpaceZ)
		<< tr("%1").arg(rawOriginX) << tr("%1").arg(rawOriginY) << tr("%1").arg(rawOriginZ)
		<< tr("%1").arg(rawHeaderSize)
		<< datatype
		<< byteOrderStr);

	dlg_openfile_sizecheck *dlg = new dlg_openfile_sizecheck (false, parent, "RAW file specs", 12, inList, inPara, NULL, f);

	if (dlg->exec() == QDialog::Accepted)
	{
		rawSizeX = dlg->getValues()[0]; rawSizeY = dlg->getValues()[1]; rawSizeZ = dlg->getValues()[2];      
		extent[0] = 0; extent[2] = 0; extent[4] = 0;
		extent[1] = rawSizeX; extent[3]= rawSizeY; extent[5] = rawSizeZ;      
		extent[1]--; extent[3]--; extent[5]--;

		rawSpaceX = dlg->getValues()[3]; rawSpaceY = dlg->getValues()[4]; rawSpaceZ = dlg->getValues()[5];
		spacing[0] = rawSpaceX; spacing[1]= rawSpaceY; spacing[2] = rawSpaceZ;

		rawOriginX = dlg->getValues()[6]; rawOriginY = dlg->getValues()[7]; rawOriginZ = dlg->getValues()[8];
		origin[0] = rawOriginX; origin[1]= rawOriginY; origin[2] = rawOriginZ;

		rawHeaderSize = dlg->getValues()[9];
		headersize = rawHeaderSize;
		fileName = f;
				
		if (dlg->getComboBoxValues()[10] == "VTK_UNSIGNED_CHAR") scalarType = VTK_UNSIGNED_CHAR;
		if (dlg->getComboBoxValues()[10] == "VTK_CHAR") scalarType = VTK_CHAR;
		if (dlg->getComboBoxValues()[10] == "VTK_UNSIGNED_SHORT") scalarType = VTK_UNSIGNED_SHORT;
		if (dlg->getComboBoxValues()[10] == "VTK_SHORT") scalarType = VTK_SHORT;
		if (dlg->getComboBoxValues()[10] == "VTK_UNSIGNED_INT") scalarType = VTK_UNSIGNED_INT;
		if (dlg->getComboBoxValues()[10] == "VTK_INT") scalarType = VTK_INT;
		if (dlg->getComboBoxValues()[10] == "VTK_FLOAT") scalarType = VTK_FLOAT;
		if (dlg->getComboBoxValues()[10] == "VTK_DOUBLE") scalarType = VTK_DOUBLE;
		
		rawScalarType = scalarType;
		if (dlg->getComboBoxValues()[11] == "Little Endian") 
			byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
		else if (dlg->getComboBoxValues()[11] == "Big Endian") 
			byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;

		rawByteOrder = byteOrder;
	}
	else return false;

	return true;
}


bool iAIO::setupPARSReader( QString f )
{
	extent[0] = 0; extent[1] = getParameterValues(f,"det_size:", 0).toInt()-1;
	extent[2] = 0; extent[3] = getParameterValues(f, "det_size:", 1).toInt()-1;
	extent[4] = 0; extent[5] = getParameterValues(f, "reco_n_proj:", 0).toInt()-1;

	spacing[0] = getParameterValues(f, "det_pitch:", 0).toDouble() / (getParameterValues(f, "geo_SD:", 0).toDouble() / getParameterValues(f, "geo_SO:", 0).toDouble());
	spacing[1] = getParameterValues(f, "det_pitch:", 1).toDouble() / (getParameterValues(f, "geo_SD:", 0).toDouble() / getParameterValues(f, "geo_SO:", 0).toDouble());
	spacing[2] = spacing[0] > spacing[1] ? spacing[1] : spacing[0];
	
	if(getParameterValues(f,"proj_datatype:",0) == "intensity") scalarType = VTK_UNSIGNED_SHORT;
	else scalarType = VTK_FLOAT;

	fileName = getParameterValues(f,"proj_filename_template_1:",0);
	QFileInfo pars(f);
	if(!QFile::exists(fileName))	{
		if ((fileName.lastIndexOf("\\") == -1) && (fileName.lastIndexOf("/") == -1)) 
			fileName = pars.canonicalPath() + "/" + fileName;
		else if (fileName.lastIndexOf("\\") > 0) 
			fileName = pars.canonicalPath() + "/" + fileName.section('\\', -1);
		else if (fileName.lastIndexOf("/") > 0)
			fileName = pars.canonicalPath() + "/" + fileName.section('/', -1);
		else
			fileName = QFileDialog::getOpenFileName(parent, tr("Specify data File (file path in PARS is wrong)"), "", tr("PRO (*.pro);;RAW files (*.raw);;"));
	}
	QFile file;
	file.setFileName(fileName);
	if(!file.open(QIODevice::ReadOnly))    return false;
	else file.close();

	return true;
}


bool iAIO::setupVGIReader( QString f )
{
	extent[1] =	getParameterValues(f,"size", 0, "[file1]", "=").toInt() ; 
	extent[3] = getParameterValues(f,"size", 1, "[file1]", "=").toInt() ; 
	extent[5] = getParameterValues(f,"size", 2, "[file1]", "=").toInt() ;
	if ((extent[1] == 0) || (extent[3] == 0) || (extent[5] == 0)) {
		extent[1] = getParameterValues(f,"Size", 0, "[file1]", "=").toInt() ;
		extent[3] = getParameterValues(f,"Size", 1, "[file1]", "=").toInt() ; 
		extent[5] = getParameterValues(f,"Size", 2, "[file1]", "=").toInt() ;
	}
	if ((extent[1] == 0) || (extent[3] == 0) || (extent[5] == 0))    return false;
	extent[1]--; extent[3]--; extent[5]--;

	spacing[0] = getParameterValues(f,"resolution", 0, "[geometry]", "=").toDouble();
	spacing[1] = getParameterValues(f,"resolution", 1, "[geometry]", "=").toDouble(); 
	spacing[2] = getParameterValues(f,"resolution", 2, "[geometry]", "=").toDouble();
	if ((spacing[0] == 0) || (spacing[1] == 0) || (spacing[2] == 0)) spacing[0] = spacing[1] = spacing[2] = 1;
	if ((spacing[1] == 0) && (spacing[2] == 0)) spacing[1] = spacing[2] = spacing[0];
	
	int elementSize = getParameterValues(f,"bitsperelement", 0, "[file1]", "=").toInt();
	if (elementSize == 0) elementSize = getParameterValues(f,"BitsPerElement", 0, "[file1]", "=").toInt();
	if (elementSize == 0)    return false;
	
	if (elementSize == 8) scalarType = VTK_UNSIGNED_CHAR;
	else if (elementSize == 16) scalarType = VTK_UNSIGNED_SHORT;
	else if (elementSize == 32)	scalarType = VTK_FLOAT;

	headersize = getParameterValues(f,"skipheader", 0, "[file1]", "=").toInt();
	if (headersize == 0) headersize = getParameterValues(f,"Skipheader", 0, "[file1]", "=").toInt();

	fileName = getParameterValues(f,"name",0, "[file1]", "=");
	if (fileName == "") fileName = getParameterValues(f,"Name",0, "[file1]", "=");
	if (fileName == "") 
		fileName = QFileDialog::getOpenFileName(parent, tr("Specify data File (file path in VGI is wrong)"), "", tr("RAW files (*.raw);;REC files (*.rec);;VOL files (*.vol);;"));
	
	QFileInfo pars(f);
	if(!QFile::exists(fileName))	{
		if ((fileName.lastIndexOf("\\") == -1) && (fileName.lastIndexOf("/") == -1)) 
			fileName = pars.canonicalPath() + "/" + fileName;
		else if (fileName.lastIndexOf("\\") > 0) 
			fileName = pars.canonicalPath() + "/" + fileName.section('\\', -1);
		else if (fileName.lastIndexOf("/") > 0)
			fileName = pars.canonicalPath() + "/" + fileName.section('/', -1);
		else
			fileName = QFileDialog::getOpenFileName(parent, tr("Specify data File (file path in VGI is wrong)"), "", tr("RAW files (*.raw);;REC files (*.rec);;VOL files (*.vol);;"));
	}

	QFile file;
	file.setFileName(fileName);
	if(!file.open(QIODevice::ReadOnly))    return false;
	else file.close();

	return true;
}


bool iAIO::writeMetaImage( )
{
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();
	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		iAConnector::ITKPixelType itkPixelType = getConnector()->GetITKPixelType();
		ITK_EXTENDED_TYPED_CALL(write_image_template, itkType, itkPixelType, 
			compression, fileName, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return false;
	}
	emit msg(tr("%1  File saved.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: "+ fileName );

	return true;
}


bool iAIO::writeSTL( )
{
	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	stlWriter->AddObserver(vtkCommand::ProgressEvent, observerProgress);
	stlWriter->SetFileName(fileName.toLatin1());
	stlWriter->SetInputData(getVtkPolyData());
	stlWriter->SetFileTypeToBinary();
	stlWriter->Write();
	emit msg(tr("%1  File saved.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: "+ fileName );
	stlWriter->ReleaseDataFlagOn();
	return true;
}


template <typename T>
void writeImageStack_template(QString const & fileName, iAProgress* p, iAConnector* con, bool comp)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<T, DIM-1> OutputImageType;
	typedef itk::ImageSeriesWriter<InputImageType, OutputImageType> SeriesWriterType;
	typename SeriesWriterType::Pointer writer = SeriesWriterType::New();

	typedef itk::NumericSeriesFileNames    NameGeneratorType;
	typename NameGeneratorType::Pointer nameGenerator = NameGeneratorType::New();

	typename InputImageType::RegionType region = dynamic_cast<InputImageType*>(con->GetITKImage())->GetLargestPossibleRegion();
	typename InputImageType::IndexType start = region.GetIndex();
	typename InputImageType::SizeType size = region.GetSize();
	nameGenerator->SetStartIndex(start[2]);
	nameGenerator->SetEndIndex(start[2] + size[2] - 1);
	nameGenerator->SetIncrementIndex(1);

	QFileInfo fi(fileName);

	if (fi.completeSuffix() == "DCM")	// should be equal to if (ioID == DCM_WRITER)
	{
		typedef itk::GDCMImageIO ImageIOType;
		ImageIOType::Pointer gdcmIO = ImageIOType::New();
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

	QString format(fi.absolutePath() + "/" + fi.baseName() + "%d." + fi.completeSuffix());
	nameGenerator->SetSeriesFormat(format.toStdString().c_str());
	writer->SetFileNames(nameGenerator->GetFileNames());
	writer->SetInput(dynamic_cast< InputImageType * > (con->GetITKImage()));
	writer->SetUseCompression(comp);
	p->Observe(writer);
	writer->Update();
}

bool iAIO::writeImageStack( )
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	try
	{
		getConnector()->SetImage(getVtkImageData());
		const ScalarPixelType pixelType = getConnector()->GetITKScalarPixelType();
		const PixelType imagePixelType = getConnector()->GetITKPixelType();
		ITK_EXTENDED_TYPED_CALL(writeImageStack_template, pixelType, imagePixelType,
			fileName, getItkProgress(), getConnector(), compression);
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return false;
	}
	emit msg(tr("%1  %2 Image Stack saved.")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(QFileInfo(fileName).completeSuffix().toUpper()));
	emit msg("  Base Filename: " + fileName);
	return true;
}

//****************************************************
//*                                                  *
//*                  2D reading                      *
//*                                                  *
//****************************************************

bool iAIO::setupStackReader( QString f )
{
	int indexRange[2] = {1, 1080};
	int digitsInIndex = 4;

	fileNamesBase = f;
	extension = "." + QFileInfo(f).suffix();
	QStringList datatype = (QStringList() <<  tr("VTK_UNSIGNED_SHORT") << tr("VTK_UNSIGNED_CHAR") <<  tr("VTK_CHAR") <<   tr("VTK_SHORT") <<  tr("VTK_INT") <<  tr("VTK_UNSIGNED_INT") <<  tr("VTK_FLOAT") <<  tr("VTK_DOUBLE"));
	QStringList inList		= (QStringList() 
		<< tr("#File Names Base") << tr("#Extension") 
		<< tr("#Number of Digits in Index")
		<< tr("#Minimum Index")  << tr("#Maximum Index")
		<< tr("#Spacing X")  << tr("#Spacing Y")  << tr("#Spacing Z")
		<< tr("#Origin X")  << tr("#Origin Y")  << tr("#Origin Z"))	<< tr("+Data Type");
	QList<QVariant> inPara	= (QList<QVariant>() 
		<< fileNamesBase << extension 
		<< tr("%1").arg(digitsInIndex) 
		<< tr("%1").arg(indexRange[0]) << tr("%1").arg(indexRange[1])
		<< tr("%1").arg(spacing[0]) << tr("%1").arg(spacing[1]) << tr("%1").arg(spacing[2])
		<< tr("%1").arg(origin[0]) << tr("%1").arg(origin[1]) << tr("%1").arg(origin[2]) << datatype);

	dlg_commoninput *dlg = new dlg_commoninput (parent, "Set file parameters", 12, inList, inPara, NULL);

	if (dlg->exec() == QDialog::Accepted){

		fileNamesBase = dlg->getText()[0];
		extension = dlg->getText()[1];
		digitsInIndex = dlg->getValues()[2];
		indexRange[0] = dlg->getValues()[3]; indexRange[1]= dlg->getValues()[4];
		spacing[0] = dlg->getValues()[5]; spacing[1]= dlg->getValues()[6]; spacing[2] = dlg->getValues()[7];
		origin[0] = dlg->getValues()[8]; origin[1]= dlg->getValues()[9]; origin[2] = dlg->getValues()[10];
		 
		if (dlg->getComboBoxValues()[11] == "VTK_UNSIGNED_CHAR") scalarType = VTK_UNSIGNED_CHAR;
		if (dlg->getComboBoxValues()[11] == "VTK_CHAR") scalarType = VTK_CHAR;
		if (dlg->getComboBoxValues()[11] == "VTK_UNSIGNED_SHORT") scalarType = VTK_UNSIGNED_SHORT;
		if (dlg->getComboBoxValues()[11] == "VTK_SHORT") scalarType = VTK_SHORT;
		if (dlg->getComboBoxValues()[11] == "VTK_UNSIGNED_INT") scalarType = VTK_UNSIGNED_INT;
		if (dlg->getComboBoxValues()[11] == "VTK_INT") scalarType = VTK_INT;
		if (dlg->getComboBoxValues()[11] == "VTK_FLOAT") scalarType = VTK_FLOAT;
		if (dlg->getComboBoxValues()[11] == "VTK_DOUBLE") scalarType = VTK_DOUBLE;

		FillFileNameArray(indexRange, digitsInIndex);
	}
	else return false;

	return true;
}


bool iAIO::readImageStack()
{
	vtkSmartPointer<vtkImageReader2> imgReader;
	switch (ioID)
	{
		case TIF_STACK_READER: imgReader = vtkSmartPointer<vtkTIFFReader>::New(); break;
		case JPG_STACK_READER: imgReader = vtkSmartPointer<vtkJPEGReader>::New(); break;
		case PNG_STACK_READER: imgReader = vtkSmartPointer<vtkPNGReader>::New(); break;
		case BMP_STACK_READER: imgReader = vtkSmartPointer<vtkBMPReader>::New(); break;
		default: emit msg(tr("%1  Invalid Image Stack IO id, aborting.")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
			return false;
	}
	imgReader->ReleaseDataFlagOn();
	imgReader->AddObserver(vtkCommand::ProgressEvent, observerProgress);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	try
	{
		imgReader->SetFileNames(fileNameArray);
		imgReader->SetDataOrigin(origin);
		imgReader->SetDataSpacing(spacing);
		imgReader->SetOutput(getVtkImageData());
		imgReader->Update();
	}
	catch (std::exception &e)
	{
		emit msg(tr("%1  An error occured while loading image stack (%2), aborting.")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(e.what()));
		return false;
	}

	printFileInfos();

	emit msg(tr("%1  Loading sequence completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	emit msg("  File: "+ fileName );
		
	return true;
}


void iAIO::printFileInfos()
{
	emit msg(tr("  Extent: [%1 %2]  [%3 %4]  [%5 %6]").arg(getVtkImageData()->GetExtent()[0])
															.arg(getVtkImageData()->GetExtent()[1])
															.arg(getVtkImageData()->GetExtent()[2])
															.arg(getVtkImageData()->GetExtent()[3])
															.arg(getVtkImageData()->GetExtent()[4])
															.arg(getVtkImageData()->GetExtent()[5]));

	emit msg(tr("  Spacing: %1 %2 %3").arg(getVtkImageData()->GetSpacing()[0])
								   .arg(getVtkImageData()->GetSpacing()[1])
								   .arg(getVtkImageData()->GetSpacing()[2]));

	emit msg(tr("  Origin: %1 %2 %3").arg(getVtkImageData()->GetOrigin()[0])
								   .arg(getVtkImageData()->GetOrigin()[1])
								   .arg(getVtkImageData()->GetOrigin()[2]));

	emit msg("  DataType: " + tr(getVtkImageData()->GetScalarTypeAsString()));

	emit msg( tr("  Components: %1").arg(getVtkImageData()->GetNumberOfScalarComponents() ) );
}


void iAIO::iosettingswriter()
{
	QSettings settings;
	settings.setValue("IO/rawSizeX", rawSizeX);
	settings.setValue("IO/rawSizeY", rawSizeY);
	settings.setValue("IO/rawSizeZ", rawSizeZ);
	settings.setValue("IO/rawSpaceX", rawSpaceX);
	settings.setValue("IO/rawSpaceY", rawSpaceY);
	settings.setValue("IO/rawSpaceZ", rawSpaceZ);
	settings.setValue("IO/rawOriginX", rawOriginX);
	settings.setValue("IO/rawOriginY", rawOriginY);
	settings.setValue("IO/rawOriginZ", rawOriginZ);
	settings.setValue("IO/rawScalar", rawScalarType);
	settings.setValue("IO/rawByte", rawByteOrder);
	settings.setValue("IO/rawHeader", rawHeaderSize);
}


void iAIO::iosettingsreader()
{
	QSettings settings;
	rawOriginX = settings.value("IO/rawOriginX").toDouble();
	rawOriginY = settings.value("IO/rawOriginY").toDouble();
	rawOriginZ = settings.value("IO/rawOriginZ").toDouble();
	rawSpaceX = settings.value("IO/rawSpaceX", 1).toDouble();	if (rawSpaceX == 0) rawSpaceX = 1;
	rawSpaceY = settings.value("IO/rawSpaceY", 1).toDouble();	if (rawSpaceY == 0) rawSpaceY = 1;
	rawSpaceZ = settings.value("IO/rawSpaceZ", 1).toDouble();	if (rawSpaceZ == 0) rawSpaceZ = 1;
	rawSizeX = settings.value("IO/rawSizeX").toInt();
	rawSizeY = settings.value("IO/rawSizeY").toInt();
	rawSizeZ = settings.value("IO/rawSizeZ").toInt();
	rawScalarType = settings.value("IO/rawScalar", 2).toInt(); // default data type: unsigned char
	rawByteOrder = settings.value("IO/rawByte", 0).toInt();    // default byte order: little endian
	rawHeaderSize = settings.value("IO/rawHeader").toInt();
}


void iAIO::printSTLFileInfos()
{
	// TODO: show this information in img properties instead of log
	emit msg(tr("  Cells: %1").arg(getVtkPolyData()->GetNumberOfCells()));
	emit msg(tr("  Points: %1").arg(getVtkPolyData()->GetNumberOfPoints()));
	emit msg(tr("  Polygons: %1").arg(getVtkPolyData()->GetNumberOfPolys()));
	emit msg(tr("  Lines: %1").arg(getVtkPolyData()->GetNumberOfLines()));
	emit msg(tr("  Strips: %1").arg(getVtkPolyData()->GetNumberOfStrips()));
	emit msg(tr("  Pieces: %1").arg(getVtkPolyData()->GetNumberOfPieces()));
}


/**
* Store transforms to file: itk::TransformFileWriterTemplate,
* but this class fails in debug mode - this is why serialization is done manually.
*/
void iAIO::saveTransformFile(QString fName, VersorRigid3DTransformType *transform) {
	
	ofstream outFile; 
	outFile.open(fName.toStdString());

	outFile 
		<< transform->GetParameters() 
		<< "\n"
		<< transform->GetFixedParameters()
		<< "\n";

	outFile.close();
}


VersorRigid3DTransformType * iAIO::readTransformFile(QString fName)
{
	std::string line; 
	ifstream inFile(fName.toStdString()); 
	if (inFile.is_open())
	{
		//read parameter
		getline(inFile, line);

		typedef VersorRigid3DTransformType::ParametersType ParametersType;
		mmFinalTransform = VersorRigid3DTransformType::New();
		
		ParametersType parameters(mmFinalTransform->GetNumberOfParameters());

		char * cstr = new char[line.size()+1]; 
		strcpy(cstr, line.c_str());
		std::vector<double> tokens; 

		char *p = strtok(cstr, " ,[]");
		while (p!=NULL)
		{
			istringstream iss(p);
			double d;
			iss >> d;
			tokens.push_back((d));
			p = strtok(NULL, " ,[]");
		}

		parameters[0] = tokens[0]; 
		parameters[1] = tokens[1];
		parameters[2] = tokens[2];
		parameters[3] = tokens[3];
		parameters[4] = tokens[4];
		parameters[5] = tokens[5];
		
		mmFinalTransform->SetParameters(parameters);

		//read fixed parameter -> center of rotation
		getline(inFile, line);
		VersorRigid3DTransformType::ParametersType fixedParameters(3);

		cstr = new char[line.size() + 1];
		strcpy(cstr, line.c_str());
		std::vector<double> fixedTokens;

		p = strtok(cstr, " ,[]");
		while (p != NULL)
		{
			istringstream iss(p);
			double d;
			iss >> d; 
			fixedTokens.push_back((d));
			p = strtok(NULL, " ,[]");
		}
		
		fixedParameters[0] = fixedTokens[0];
		fixedParameters[1] = fixedTokens[1];
		fixedParameters[2] = fixedTokens[2];

		mmFinalTransform->SetFixedParameters(fixedParameters);

		inFile.close();

	} else {
		addMsg(tr("unable to load transform file"));
	}

	return mmFinalTransform; 
	
}


QString iAIO::getAdditionalInfo()
{
	return m_additionalInfo;
}


void iAIO::setAdditionalInfo(QString const & additionalInfo)
{
	m_additionalInfo = additionalInfo;
}


QString iAIO::getFileName()
{
	return fileName;
}
