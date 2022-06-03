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

#include "defines.h"
#include "iAAmiraMeshIO.h"
#include "iAConnector.h"
#include "iALog.h"
#include "iAExceptionThrowingErrorObserver.h"
#include "iAExtendedTypedCallHelper.h"
#include "iAFileUtils.h"
#include "iAJobListView.h"
#include "iAModalityList.h"
#include "iAOIFReader.h"
#include "iAParameterDlg.h"
#include "iAProgress.h"
#include "iARawFileParamDlg.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"

#include "iAFilter.h"
#include "iAFilterRegistry.h"

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
#include <vtkXMLImageDataReader.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkRectilinearGrid.h>
#include <vtkPointData.h>

#include <QApplication>
#include <QFileDialog>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QTextStream>


#ifdef USE_HDF5
// for now, let's use HDF5 1.10 API:
#define H5_USE_110_API
#include <hdf5.h>
#include <QStack>
#include <QStandardItemModel>
#endif

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
		m["PNG"] = MHD_READER;
		m["NIA"] = MHD_READER;
		m["NII"] = MHD_READER;
		m["GZ"] = MHD_READER;  // actually, only nii.gz and img.gz...
		m["HDR"] = MHD_READER;
		m["IMG"] = MHD_READER;
		m["OIF"] = OIF_READER;
		m["AM"] = AM_READER;
		m["VTI"] = VTI_READER;
		m["VTK"] = VTK_READER;
		m["MOD"] = PROJECT_READER;
		m["IAPROJ"] = PROJECT_READER;
#ifdef USE_HDF5
		m["HDF5"] = HDF5_READER;
		m["H5"] = HDF5_READER;
		m["HE5"] = HDF5_READER;
		m["NC"] = HDF5_READER;
		m["CDF"] = HDF5_READER;
		m["MAT"] = HDF5_READER;
#endif
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
		m["AM"] = AM_WRITER;
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

	void addSeriesParameters(iAParameterDlg::ParamListT& params, QString const& base, QString const& ext, int digits, int const * index)
	{
		addParameter(params, FileNameBase, iAValueType::String, base);
		addParameter(params, Extension, iAValueType::String, ext);
		addParameter(params, NumDigits, iAValueType::Discrete, digits);
		addParameter(params, MinimumIndex, iAValueType::Discrete, index[0]);
		addParameter(params, MaximumIndex, iAValueType::Discrete, index[1]);
	}
}

#ifdef USE_HDF5

namespace
{
	const int InvalidHDF5Type = -1;

	QString MapHDF5TypeToString(H5T_class_t hdf5Type)
	{
		switch (hdf5Type)
		{
			case H5T_NO_CLASS  : return QString("No Class");
			case H5T_INTEGER   : return QString("Integer");
			case H5T_FLOAT	   : return QString("Float");
			case H5T_TIME	   : return QString("Time");
			case H5T_STRING	   : return QString("String");
			case H5T_BITFIELD  : return QString("Bitfield");
			case H5T_OPAQUE	   : return QString("Opaque");
			case H5T_COMPOUND  : return QString("Compound");
			case H5T_REFERENCE : return QString("Reference");
			case H5T_ENUM	   : return QString("Enum");
			case H5T_VLEN	   : return QString("VLen");
			case H5T_ARRAY     : return QString("Array");
			default: return QString("Unknown");
		}
	}
	int GetNumericVTKTypeFromHDF5Type(H5T_class_t hdf5Type, size_t numBytes, H5T_sign_t sign)
	{
		switch (hdf5Type)
		{
		case H5T_INTEGER: {
			switch (numBytes)
			{
			case 1: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_CHAR      : VTK_SIGNED_CHAR;
			case 2: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_SHORT     : VTK_SHORT;
			case 4: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_INT       : VTK_INT;
			case 8: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_LONG_LONG : VTK_LONG_LONG;
			default: return InvalidHDF5Type;
			}
		}
		case H5T_FLOAT: {
			switch (numBytes)
			{
				case 4:  return VTK_FLOAT;
				case 8:  return VTK_DOUBLE;
				default: return InvalidHDF5Type;
			}
		}
		default: return InvalidHDF5Type;
		}
	}

	hid_t GetHDF5ReadType(H5T_class_t hdf5Type, size_t numBytes, H5T_sign_t sign)
	{
		switch (hdf5Type)
		{
		case H5T_INTEGER: {
			switch (numBytes)
			{
			case 1: return (sign == H5T_SGN_NONE) ? H5T_NATIVE_UCHAR : H5T_NATIVE_SCHAR;
			case 2: return (sign == H5T_SGN_NONE) ? H5T_NATIVE_USHORT : H5T_NATIVE_SHORT;
			case 4: return (sign == H5T_SGN_NONE) ? H5T_NATIVE_UINT : H5T_NATIVE_INT;
			case 8: return (sign == H5T_SGN_NONE) ? H5T_NATIVE_ULLONG : H5T_NATIVE_LLONG;
			default: return InvalidHDF5Type;
			}
		}
		case H5T_FLOAT: {
			switch (numBytes)
			{
			case 4:  return H5T_NATIVE_FLOAT;
			case 8:  return H5T_NATIVE_DOUBLE;
			default: return InvalidHDF5Type;
			}
		}
		default: return InvalidHDF5Type;
		}
	}

	// typedef herr_t(*H5E_walk2_t)(unsigned n, const H5E_error2_t *err_desc, void *client_data)
	herr_t errorfunc(unsigned /*n*/, const H5E_error2_t *err, void * /*client_data*/)
	{
		/*
		hid_t       cls_id;     class ID
		hid_t       maj_num;	major error ID
		hid_t       min_num;	minor error number
		unsigned	line;		line in file where error occurs
		const char	*func_name; function in which error occurred
		const char	*file_name;	file in which error occurred
		const char	*desc;
		*/
		LOG(lvlError, QString("HDF5 error: class=%1 maj_num=%2(%3) min_num=%4(%5) file=%6:%7 func=%8 desc=%9")
			.arg(err->cls_id)
			.arg(err->maj_num)
			.arg(H5Eget_major(err->maj_num))
			.arg(err->min_num)
			.arg(H5Eget_minor(err->min_num))
			.arg(err->file_name)
			.arg(err->line)
			.arg(err->func_name)
			.arg(err->desc));
		return 0;
	}

	void printHDF5ErrorsToConsole()
	{
		hid_t err_stack = H5Eget_current_stack();
		/*herr_t walkresult = */ H5Ewalk(err_stack, H5E_WALK_UPWARD, errorfunc, nullptr);
	}
}


void iAIO::readHDF5File()
{
	if (m_hdf5Path.size() < 2)
	{
		throw std::runtime_error("HDF5 file: Insufficient path length.");
	}
	hid_t file = H5Fopen( getLocalEncodingFileName(m_fileName).c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	m_hdf5Path.removeLast();
	hid_t loc_id = file;
	QStack<hid_t> openGroups;
	while (m_hdf5Path.size() > 1)
	{
		QString name = m_hdf5Path.last();
		m_hdf5Path.removeLast();
		loc_id = H5Gopen(file, name.toStdString().c_str(), H5P_DEFAULT);  // TODO: check which encoding HDF5 internal strings have!
		openGroups.push(loc_id);
	}

	hid_t dataset_id = H5Dopen(loc_id, m_hdf5Path[0].toStdString().c_str(), H5P_DEFAULT);
	hid_t space = H5Dget_space(dataset_id);
	int rank = H5Sget_simple_extent_ndims(space);
	hsize_t * hdf5Dims = new hsize_t[rank];
	hsize_t * maxdims = new hsize_t[rank];
	int status = H5Sget_simple_extent_dims(space, hdf5Dims, maxdims);
	//H5S_class_t hdf5Class = H5Sget_simple_extent_type(space);
	hid_t type_id = H5Dget_type(dataset_id);
	H5T_class_t hdf5Type = H5Tget_class(type_id);
	size_t numBytes = H5Tget_size(type_id);
	H5T_order_t order = H5Tget_order(type_id);
	H5T_sign_t sign = H5Tget_sign(type_id);
	int vtkType = GetNumericVTKTypeFromHDF5Type(hdf5Type, numBytes, sign);
	H5Tclose(type_id);						// class=%2;
	QString caption = QString("Dataset: %1; type=%2 (%3 bytes, order %4, %5); rank=%6; ")
		.arg(m_hdf5Path[0])
		//.arg(MapHDF5ClassToString(hdf5Class))
		.arg(MapHDF5TypeToString(hdf5Type))
		.arg(numBytes)
		.arg((order == H5T_ORDER_LE) ? "LE" : "BE")
		.arg((sign == H5T_SGN_NONE) ? "unsigned" : "signed")
		.arg(rank);
	for (int i = 0; i < rank; ++i)
	{
		caption += QString::number(hdf5Dims[i]);
		if (i < rank - 1) caption += " x ";
	}
	//LOG(lvlInfo, caption);
	status = H5Sclose(space);
	if (vtkType == InvalidHDF5Type)
	{
		throw std::runtime_error("HDF5: Can't load a dataset of this data type!");
	}

	// actual reading of data:
	//status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, );

	int dim[3];
	for (int i = 0; i < 3; ++i)
	{
		dim[i] = (i < rank) ? hdf5Dims[i] : 1;
	}
	unsigned char * raw_data = new unsigned char[numBytes * dim[0] * dim[1] * dim[2] ];
	status = H5Dread(dataset_id, GetHDF5ReadType(hdf5Type, numBytes, sign), H5S_ALL, H5S_ALL, H5P_DEFAULT, raw_data);
	if (status < 0)
	{
		printHDF5ErrorsToConsole();
		throw std::runtime_error("Reading dataset failed!");
	}
	H5Dclose(dataset_id);
	while (openGroups.size() > 0)
	{
		H5Gclose(openGroups.pop());
	}
	H5Fclose(file);

	vtkSmartPointer<vtkImageImport> imgImport = vtkSmartPointer<vtkImageImport>::New();
	imgImport->SetDataSpacing(m_hdf5Spacing[2], m_hdf5Spacing[1], m_hdf5Spacing[0]);
	imgImport->SetDataOrigin(0, 0, 0);
	imgImport->SetWholeExtent(0, dim[2]-1, 0, dim[1]-1, 0, dim[0]-1);
	imgImport->SetDataExtentToWholeExtent();
	imgImport->SetDataScalarType(vtkType);
	imgImport->SetNumberOfScalarComponents(1);
	imgImport->SetImportVoidPointer(raw_data, 0);
	imgImport->Update();
	getConnector()->setImage(imgImport->GetOutput());
	getConnector()->modified();
	postImageReadActions();
}
#endif

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
			case AM_READER: {
				vtkSmartPointer<vtkImageData> img = iAAmiraMeshIO::Load(fileName());
				getConnector()->setImage(img);
				getConnector()->modified();
				postImageReadActions();
				break;
			}
			case AM_WRITER: {
				iAAmiraMeshIO::Write(fileName(), getVtkImageData());
				break;
			}
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
			case VTI_READER: {
				vtkSmartPointer<vtkXMLImageDataReader> reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
				reader->SetFileName( getLocalEncodingFileName(fileName()).c_str() );
				reader->Update();
				getConnector()->setImage(reader->GetOutput());
				getConnector()->modified();
				postImageReadActions();
				break;
			}
	#ifdef USE_HDF5
			case HDF5_READER:
				if (m_isITKHDF5)
				{
					readMetaImage();
				}
				else
				{
					readHDF5File();
				}
				break;
	#endif
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


#ifdef USE_HDF5
namespace
{

struct opdata {
	QStandardItem* item;
	unsigned        recurs;         /* Recursion level.  0=root */
	struct opdata   *prev;          /* Pointer to previous opdata */
	haddr_t         addr;           /* Group address */
};

const int DATASET = 0;
const int GROUP = 1;
const int OTHER = 2;

//! This function recursively searches the linked list of opdata structures
//! for one whose address matches target_addr.  Returns 1 if a match is
//! found, and 0 otherwise.
int group_check(struct opdata *od, haddr_t target_addr)
{
	if (od->addr == target_addr)
	{
		return 1;       /* Addresses match */
	}
	else if (!od->recurs)
	{
		return 0;       /* Root group reached with no matches */
	}
	else
	{
		return group_check(od->prev, target_addr);
	}
	/* Recursively examine the next node */
}

herr_t op_func(hid_t loc_id, const char *name, const H5L_info_t * /*info*/,
	void *operator_data)
{
	herr_t          status, return_val = 0;
	H5O_info_t      infobuf;
	struct opdata   *od = (struct opdata *) operator_data;
	status = H5Oget_info_by_name(loc_id, name, &infobuf, H5P_DEFAULT);
	if (status < 0)
	{
		LOG(lvlWarn, QString("H5Oget_info_by_name failed with code %1!").arg(status));
	}
	QString caption;
	//bool group = false;
	int vtkType = -1;
	int rank = 0;
	switch (infobuf.type)
	{
	case H5O_TYPE_GROUP:
		caption = QString("Group: %1").arg(name);
		//group = true;
		break;
	case H5O_TYPE_DATASET:
		{
		hid_t dset = H5Dopen(loc_id, name, H5P_DEFAULT);
		if (dset == -1)
		{
			caption = QString("Dataset %1; unable to determine size").arg(name);
			break;
		}
		hid_t space = H5Dget_space(dset);
		rank = H5Sget_simple_extent_ndims(space);
		hsize_t * dims = new hsize_t[rank];
		hsize_t * maxdims = new hsize_t[rank];
		status = H5Sget_simple_extent_dims(space, dims, maxdims);
		//H5S_class_t hdf5Class = H5Sget_simple_extent_type(space);
		hid_t type_id = H5Dget_type(dset);
		H5T_class_t hdf5Type = H5Tget_class(type_id);
		size_t numBytes = H5Tget_size(type_id);
		H5T_order_t order = H5Tget_order(type_id);
		H5T_sign_t sign = H5Tget_sign(type_id);
		vtkType = GetNumericVTKTypeFromHDF5Type(hdf5Type, numBytes, sign);
		H5Tclose(type_id);
		caption = QString("Dataset: %1; type=%2 (%3 bytes, order %4, %5); rank=%6; ")
			.arg(name)
			//.arg(MapHDF5ClassToString(hdf5Class))
			.arg(MapHDF5TypeToString(hdf5Type))
			.arg(numBytes)
			.arg((order == H5T_ORDER_LE) ? "LE" : "BE")
			.arg((sign == H5T_SGN_NONE) ? "unsigned" : "signed")
			.arg(rank);
		for (int i = 0; i < rank; ++i)
		{
			caption += QString("%1%2").arg(dims[i]).arg((dims[i] != maxdims[i]) ? QString("%1").arg(maxdims[i]) : QString());
			if (i < rank - 1) caption += " x ";
		}
		status = H5Sclose(space);
		status = H5Dclose(dset);
		break;
		}
	case H5O_TYPE_NAMED_DATATYPE:
		caption = QString("Datatype: %1").arg(name);
		break;
	default:
		caption = QString("Unknown: %1").arg(name);
	}
	QStandardItem* newItem = new QStandardItem(caption);
	od->item->appendRow(newItem);

	switch (infobuf.type) {
	case H5O_TYPE_GROUP:
		if (group_check(od, infobuf.addr))
		{
			caption += QString(" (Warning: Loop detected!)");
		}
		else
		{
			struct opdata nextod;
			nextod.item = newItem;
			nextod.recurs = od->recurs + 1;
			nextod.prev = od;
			nextod.addr = infobuf.addr;
			return_val = H5Literate_by_name(loc_id, name, H5_INDEX_NAME,
				H5_ITER_NATIVE, nullptr, op_func, (void *)&nextod,
				H5P_DEFAULT);
		}
		newItem->setData(GROUP, Qt::UserRole + 1);
		break;
	case H5O_TYPE_DATASET:
		newItem->setData(DATASET, Qt::UserRole + 1);
		break;
	default:
		newItem->setData(OTHER, Qt::UserRole + 1);
		break;
	}
	newItem->setData(QString("%1").arg(name), Qt::UserRole + 2);
	newItem->setData(vtkType, Qt::UserRole + 3);
	newItem->setData(rank, Qt::UserRole + 4);
	return return_val;
}

bool HDF5GroupExists(hid_t file_id, const char * name)
{
	hid_t loc_id = H5Gopen(file_id, name, H5P_DEFAULT);
	bool result = loc_id > 0;
	if (result)
	{
		H5Gclose(loc_id);
	}
	return result;
}

bool HDF5DatasetExists(hid_t file_id, const char * name)
{
	hid_t loc_id = H5Dopen(file_id, name, H5P_DEFAULT);
	bool result = loc_id > 0;
	if (result)
	{
		H5Dclose(loc_id);
	}
	return result;
}

bool IsHDF5ITKImage(hid_t file_id)
{
	return HDF5GroupExists(file_id, "ITKImage") && HDF5DatasetExists(file_id, "ITKVersion");
}

}

#include "qthelper/iAQTtoUIConnector.h"
#include "ui_OpenHDF5.h"
typedef iAQTtoUIConnector<QDialog, Ui_dlgOpenHDF5> OpenHDF5Dlg;
#endif


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
		case AM_READER:
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
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case VTI_READER:
			m_fileName = f; break;
#ifdef USE_HDF5
		case HDF5_READER:
		{
			m_fileName = f;
			hid_t file_id = H5Fopen( getLocalEncodingFileName(m_fileName).c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
			if (file_id < 0)
			{
				printHDF5ErrorsToConsole();
				LOG(lvlError, "H5open returned value < 0!");
				return false;
			}
			m_isITKHDF5 = IsHDF5ITKImage(file_id);
			if (m_isITKHDF5)
			{
				H5Fclose(file_id);
				return true;
			}

			QStandardItemModel* model = new QStandardItemModel();
			model->setHorizontalHeaderLabels(QStringList() << "HDF5 Structure");
			QStandardItem* rootItem = new QStandardItem(QFileInfo(m_fileName).fileName() + "/");
			rootItem->setData(GROUP, Qt::UserRole + 1);
			rootItem->setData(m_fileName, Qt::UserRole + 2);
			model->appendRow(rootItem);

			H5O_info_t infobuf;
			H5Oget_info(file_id, &infobuf);
			struct opdata   od;
			od.item = rootItem;
			od.recurs = 0;
			od.prev = nullptr;
			od.addr = infobuf.addr;
			H5Literate(file_id, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, op_func, (void *)&od);
			H5Fclose(file_id);

			// check if maybe only one dataset is contained in the file anyway:
			QStandardItem* curItem = rootItem;
			while (curItem)
			{
				if (curItem->rowCount() > 1)
				{
					curItem = nullptr;
					break;
				}
				curItem = curItem->child(0);
				assert(curItem);
				if (curItem->data(Qt::UserRole + 1) == DATASET)
					break;
			}
			QModelIndex idx;
			if (curItem && curItem->data(Qt::UserRole + 1) == DATASET)
			{
				LOG(lvlInfo, "File only contains one dataset, loading that with default spacing of 1,1,1!");
				idx = curItem->index();
				m_hdf5Spacing[0] = 1;
				m_hdf5Spacing[1] = 1;
				m_hdf5Spacing[2] = 1;
			}
			else
			{
				OpenHDF5Dlg dlg;
				dlg.setWindowTitle(QString("Open HDF5").arg(m_fileName));
				dlg.tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
				dlg.tree->setModel(model);
				if (dlg.exec() != QDialog::Accepted)
				{
					addMsg("Dataset selection aborted.");
					return false;
				}
				idx = dlg.tree->currentIndex();
				bool okX, okY, okZ;
				m_hdf5Spacing[0] = dlg.edSpacingX->text().toDouble(&okX);
				m_hdf5Spacing[1] = dlg.edSpacingY->text().toDouble(&okY);
				m_hdf5Spacing[2] = dlg.edSpacingZ->text().toDouble(&okZ);
				if (!(okX && okY && okZ))
				{
					addMsg("Invalid spacing (has to be a valid floating point number)!");
					return false;
				}
			}
			if (idx.data(Qt::UserRole + 1) != DATASET)
			{
				addMsg("You have to select a dataset!");
				return false;
			}
			if (idx.data(Qt::UserRole + 3).toInt() == -1)
			{
				addMsg("Can't read datasets of this data type!");
				return false;
			}
			if (idx.data(Qt::UserRole + 4).toInt() < 1 || idx.data(Qt::UserRole + 4).toInt() > 3)
			{
				addMsg(QString("The rank (number of dimensions) of the dataset must be between 1 and 3 (was %1)").arg(idx.data(Qt::UserRole + 4).toInt()));
			}
			do
			{
				m_hdf5Path.append(idx.data(Qt::UserRole+2).toString());
				idx = idx.parent();
			}
			while (idx != QModelIndex());
			LOG(lvlInfo, QString("Path: %1").arg(m_hdf5Path.size()));
			for (int i = 0; i < m_hdf5Path.size(); ++i)
			{
				LOG(lvlInfo, QString("    %1").arg(m_hdf5Path[i]));
			}
			if (m_hdf5Path.size() < 2)
			{
				addMsg("Invalid selection!");
				return false;
			}
			return true;
		}
#endif
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
	QMap<QString, QVariant> parameters;
	parameters["Value To Replace"] = 65533;
	parameters["Replacement"] = 0;
	filter->run(parameters);

	auto dataTypeConversion = iAFilterRegistry::filter("Datatype Conversion");
	if (!filter)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Datatype Conversion' filter, but filter could not be found!")
				.arg(m_fileName));
		return;
	}

	dataTypeConversion->addInput(filter->output(0)->itkImage(), "");
	QMap<QString, QVariant> parametersConversion;
	parametersConversion["Data Type"] = "32 bit floating point number (7 digits, float)";
	parametersConversion["Rescale Range"] = false;
	parametersConversion["Automatic Input Range"] = true;
	parametersConversion["Use Full OutputRange"] = true;
	dataTypeConversion->run(parametersConversion);

	auto filterScale = iAFilterRegistry::filter("Shift and Scale");
	if (!filter)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Shift and Scale' filter, but filter could not be found!")
				.arg(m_fileName));
		return;
	}
	filterScale->addInput(dataTypeConversion->output(0)->itkImage(), "");
	QMap<QString, QVariant> parametersScale;
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
	iAParameterDlg::ParamListT params;
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
	iAParameterDlg::ParamListT params;
	addSeriesParameters(params, m_fileNamesBase, m_extension, digitsInIndex, indexRange);
	iARawFileParamDlg dlg(f, m_parent, "RAW file specs", params, m_rawFileParams);
	if (!dlg.accepted())
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

bool iAIO::setupRAWReader( QString const & f )
{
	m_fileName = f;
	iARawFileParamDlg dlg(f, m_parent, "RAW file specs", iAParameterDlg::ParamListT(), m_rawFileParams);
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
	iAParameterDlg::ParamListT params;
	addSeriesParameters(params, m_fileNamesBase, m_extension, digits, indexRange);
	addParameter(params, "Step", iAValueType::Discrete, 1);
	addParameter(params, "Spacing X", iAValueType::Continuous, m_rawFileParams.m_spacing[0]);
	addParameter(params, "Spacing Y", iAValueType::Continuous, m_rawFileParams.m_spacing[1]);
	addParameter(params, "Spacing Z", iAValueType::Continuous, m_rawFileParams.m_spacing[2]);
	addParameter(params, "Origin X", iAValueType::Continuous, m_rawFileParams.m_origin[0]);
	addParameter(params, "Origin Y", iAValueType::Continuous, m_rawFileParams.m_origin[1]);
	addParameter(params, "Origin Z", iAValueType::Continuous, m_rawFileParams.m_origin[2]);
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
