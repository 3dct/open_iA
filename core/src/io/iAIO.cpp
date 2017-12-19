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
#include "iAConsole.h"
#include "iAExceptionThrowingErrorObserver.h"
#include "iAExtendedTypedCallHelper.h"
#include "iAObserverProgress.h"
#include "iAOIFReader.h"
#include "iAProgress.h"
#include "iAVolumeStack.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"

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

#ifdef USE_HDF5
#include <hdf5.h>
#include <QStack>
#endif

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


const QString iAIO::VolstackExtension(".volstack");


template<class T>
void read_raw_image_template (unsigned long headerSize,
	int byteOrder, int* extent, double* spacing, double* origin,
	QString const & fileName, iAProgress* progress, iAConnector* image)
{
	typedef itk::RawImageIO<T, DIM> RawImageIOType;
	auto io = RawImageIOType::New();
	io->SetFileName(fileName.toLatin1().data());
	io->SetHeaderSize(headerSize);
	for(int i=0; i<DIM; i++)
	{
		io->SetDimensions(i, extent[2*i+1] + 1);
		io->SetSpacing(i, spacing[i]);
		io->SetOrigin(i, origin[i]);
	}

	if (byteOrder == VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN)
		io->SetByteOrderToLittleEndian();
	else
		io->SetByteOrderToBigEndian();

	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	auto reader = ReaderType::New();
	reader->SetFileName(fileName.toLatin1().data());
	reader->SetImageIO(io);
	progress->Observe( reader );
	reader->Modified();
	reader->Update();
	image->SetImage(reader->GetOutput());
	image->Modified();
	reader->ReleaseDataFlagOn();
}


template<class T>
void read_image_template(QString const & fileName, iAProgress* progress, iAConnector* image  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;
	auto reader = ReaderType::New();
	reader->SetFileName(fileName.toLatin1().data());
	progress->Observe( reader );
	reader->Update();
	image->SetImage(reader->GetOutput());
	image->Modified();
	reader->ReleaseDataFlagOn();
}


template<class T>
void write_image_template(bool compression, QString const & fileName,
	iAProgress* progress, iAConnector* image  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::ImageFileWriter<InputImageType> WriterType;
	auto writer = WriterType::New();
	writer->SetFileName(fileName.toLatin1().data());
	writer->SetInput( dynamic_cast< InputImageType * > ( image->GetITKImage() ) );
	writer->SetUseCompression(compression);
	progress->Observe( writer );
	writer->Update();
	writer->ReleaseDataFlagOn();
}


iAIO::iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *par,
		vector<vtkSmartPointer<vtkImageData> > * volumes, vector<QString> * fileNames)
	: iAAlgorithm( "IO", i, p, logger, par ),
	m_volumes(volumes),
	m_fileNames_volstack(fileNames)
{
	init(parent);
}


iAIO::iAIO( iALogger* logger, QObject *parent /*= 0*/, vector<vtkSmartPointer<vtkImageData> > * volumes /*= 0*/,
		vector<QString> * fileNames /*= 0*/ )
	: iAAlgorithm( "IO", 0, 0, logger, parent),
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
	headersize = 0;
	scalarType = 0;
	byteOrder = 1;
	ioID = 0;
	iosettingsreader();
	observerProgress = iAObserverProgress::New();
}


iAIO::~iAIO()
{
	fileNameArray->Delete();
	if (observerProgress) observerProgress->Delete();
}

#ifdef USE_HDF5
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

namespace
{
	const int InvalidHDF5Type = -1;
}

int GetNumericVTKTypeFromHDF5Type(H5T_class_t hdf5Type, size_t numBytes, H5T_sign_t sign)
{
	switch (hdf5Type)
	{
	case H5T_INTEGER: {
		switch (numBytes)
		{
		case 1: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_CHAR : VTK_CHAR;
		case 2: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_SHORT : VTK_SHORT;
		case 4: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_INT : VTK_INT;
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
herr_t errorfunc(unsigned n, const H5E_error2_t *err, void *client_data)
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
	DEBUG_LOG(QString("HDF5 error: class=%1 maj_num=%2(%3) min_num=%4(%5) file=%6:%7 func=%8 desc=%9")
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
	herr_t walkresult = H5Ewalk(err_stack, H5E_WALK_UPWARD, errorfunc, NULL);
}

#include <vtkImageImport.h>

void iAIO::loadHDF5File()
{
	if (m_hdf5Path.size() < 2)
	{
		throw std::runtime_error("HDF5 file: Insufficient path length.");
	}
	hid_t file = H5Fopen(fileName.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	m_hdf5Path.removeLast();
	hid_t loc_id = file;
	QStack<hid_t> openGroups;
	while (m_hdf5Path.size() > 1)
	{
		QString name = m_hdf5Path.last();
		m_hdf5Path.removeLast();
		loc_id = H5Gopen(file, name.toStdString().c_str(), H5P_DEFAULT);
		openGroups.push(loc_id);
	}

	hid_t dataset_id = H5Dopen(loc_id, m_hdf5Path[0].toStdString().c_str(), H5P_DEFAULT);
	hid_t space = H5Dget_space(dataset_id);
	int rank = H5Sget_simple_extent_ndims(space);
	hsize_t * hdf5Dims = new hsize_t[rank];
	hsize_t * maxdims = new hsize_t[rank];
	int status = H5Sget_simple_extent_dims(space, hdf5Dims, maxdims);
	H5S_class_t hdf5Class = H5Sget_simple_extent_type(space);
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
		caption += QString("%1%2").arg(hdf5Dims[i]).arg((hdf5Dims[i] != maxdims[i]) ? QString("%1").arg(maxdims[i]) : QString());
		if (i < rank - 1) caption += " x ";
	}
	DEBUG_LOG(caption);
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
	imgImport->SetDataSpacing(m_hdf5Spacing[0], m_hdf5Spacing[1], m_hdf5Spacing[2]);
	imgImport->SetDataOrigin(0, 0, 0);
	imgImport->SetWholeExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
	imgImport->SetDataExtentToWholeExtent();
	imgImport->SetDataScalarType(vtkType);
	imgImport->SetNumberOfScalarComponents(1);
	imgImport->SetImportVoidPointer(raw_data, 0);
	imgImport->Update();
	getConnector()->SetImage(imgImport->GetOutput());
	getConnector()->Modified();
	postImageReadActions();
}
#endif

void iAIO::run()
{
	qApp->processEvents();
	try
	{
		switch (ioID)
		{
			case MHD_WRITER:
				writeMetaImage(getVtkImageData(), fileName)); break;
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
			case RAW_READER:
			case PARS_READER:
			case VGI_READER:
				readImageData(); break;
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
			case NRRD_READER:
				readNRRD(); break;
			case OIF_READER: {
				IO::OIF::Reader r;
				r.read(getFileName(), getConnector(), m_channel, m_volumes);
				//if (!m_volumes)
				{
					postImageReadActions();
				}
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
				break;
			}
			case AM_WRITER: {
				iAAmiraMeshIO::Write(getFileName(), getVtkImageData());
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
					loadHDF5File();
				}
				break;
	#endif
			case UNKNOWN_READER:
			default:
				addMsg(tr("  unknown reader type"));
		}
		if ((ioID == MHD_WRITER) || (ioID == STL_WRITER) || (ioID == TIF_STACK_WRITER)
			|| (ioID == JPG_STACK_WRITER) || (ioID == PNG_STACK_WRITER)|| (ioID == BMP_STACK_WRITER) || (ioID ==DCM_WRITER) )
			emit done(true);
		else emit done();
	}
	catch (std::exception & e)
	{
		addMsg(tr("%1  IO operation failed: %3").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(e.what()));
	}
}


#ifdef USE_HDF5
#include <QTextEdit>
#include <QTreeView>
#include <QStandardItemModel>

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

/**
This function recursively searches the linked list of
opdata structures for one whose address matches
target_addr.  Returns 1 if a match is found, and 0
otherwise. */
int group_check(struct opdata *od, haddr_t target_addr)
{
	if (od->addr == target_addr)
		return 1;       /* Addresses match */
	else if (!od->recurs)
		return 0;       /* Root group reached with no matches */
	else
		return group_check(od->prev, target_addr);
	/* Recursively examine the next node */
}

herr_t op_func(hid_t loc_id, const char *name, const H5L_info_t *info,
	void *operator_data)
{
	herr_t          status, return_val = 0;
	H5O_info_t      infobuf;
	struct opdata   *od = (struct opdata *) operator_data;
	status = H5Oget_info_by_name(loc_id, name, &infobuf, H5P_DEFAULT);
	QString caption;
	bool group = false;
	int vtkType = -1;
	int rank = 0;
	switch (infobuf.type) {
	case H5O_TYPE_GROUP:
		caption = QString("Group: %1").arg(name);
		group = true;
		break;
	case H5O_TYPE_DATASET: {
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
		int status = H5Sget_simple_extent_dims(space, dims, maxdims);
		H5S_class_t hdf5Class = H5Sget_simple_extent_type(space);
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
				H5_ITER_NATIVE, NULL, op_func, (void *)&nextod,
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

#include "iAQTtoUIConnector.h"
#include "ui_OpenHDF5.h"
typedef iAQTtoUIConnector<QDialog, Ui_dlgOpenHDF5> OpenHDF5Dlg;
#endif

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
#ifdef USE_HDF5
		case HDF5_READER:
		{
			fileName = f;
			hid_t file_id = H5Fopen(fileName.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
			if (file_id < 0)
			{
				printHDF5ErrorsToConsole();
				DEBUG_LOG("H5open returned value < 0!");
				return false;
			}
			m_isITKHDF5 = IsHDF5ITKImage(file_id);
			if (m_isITKHDF5)
			{
				H5Fclose(file_id);
				return true;
			}
			OpenHDF5Dlg dlg;
			dlg.setWindowTitle(QString("Open HDF5").arg(fileName));
			QStandardItemModel* model = new QStandardItemModel();
			model->setHorizontalHeaderLabels(QStringList() << "HDF5 Structure");
			dlg.tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
			QStandardItem* rootItem = new QStandardItem(QFileInfo(fileName).fileName() + "/");
			rootItem->setData(GROUP, Qt::UserRole + 1);
			rootItem->setData(fileName, Qt::UserRole + 2);
			model->appendRow(rootItem);

			H5O_info_t infobuf;
			H5Oget_info(file_id, &infobuf);
			struct opdata   od;
			od.item = rootItem;
			od.recurs = 0;
			od.prev = NULL;
			od.addr = infobuf.addr;
			H5Literate(file_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, op_func, (void *)&od);
			H5Fclose(file_id);

			dlg.tree->setModel(model);
			if (dlg.exec() != QDialog::Accepted)
			{
				addMsg("Dataset selection aborted.");
				return false;
			}
			QModelIndex idx = dlg.tree->currentIndex();
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
			DEBUG_LOG(QString("Path: %1").arg(m_hdf5Path.size()));
			for (int i = 0; i < m_hdf5Path.size(); ++i)
			{
				DEBUG_LOG(QString("    %1").arg(m_hdf5Path[i]));
			}
			if (m_hdf5Path.size() < 2)
			{
				addMsg("Invalid selection!");
				return false;
			}
			bool okX, okY, okZ;
			m_hdf5Spacing[0] = dlg.edSpacingX->text().toDouble(&okX);
			m_hdf5Spacing[1] = dlg.edSpacingY->text().toDouble(&okY);
			m_hdf5Spacing[2] = dlg.edSpacingZ->text().toDouble(&okZ);
			if (!(okX && okY && okZ))
			{
				addMsg("Invalid spacing (has to be a valid floating point number)!");
				return false;
			}
			return true;
		}
#endif
		case UNKNOWN_READER:
		default:
			addMsg(tr("  unknown IO type"));
			return false;
	}
	return true;
}


void iAIO::readNRRD()
{
	typedef itk::Vector<signed short, 2>	VectorType;
	typedef itk::Image<VectorType, DIM>		DiffusionImageType;
	typedef DiffusionImageType::Pointer		DiffusionImagePointer;
	typedef itk::ImageFileReader<DiffusionImageType> FileReaderType;
	auto reader = FileReaderType::New();
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


/**
 * \brief	This function reads a series of DICOM images.
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
	nameGenerator->SetDirectory(f_dir.canonicalPath().toStdString());
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
	image->SetImage(reader->GetOutput());
	image->Modified();
	postImageReadActions();
	iosettingswriter();
}


void iAIO::loadMetaImageFile(QString const & fileName)
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	auto imageIO = itk::ImageIOFactory::CreateImageIO(fileName.toLatin1(), itk::ImageIOFactory::ReadMode);
	imageIO->SetFileName(fileName.toLatin1());
	imageIO->ReadImageInformation();
	const ScalarPixelType pixelType = imageIO->GetComponentType();
	const PixelType imagePixelType = imageIO->GetPixelType();
	ITK_EXTENDED_TYPED_CALL(read_image_template, pixelType, imagePixelType,
		fileName, getItkProgress(), getConnector());
}


void iAIO::readVolumeMHDStack()
{
	if (fileNameArray->GetSize() == 0)
		throw std::runtime_error("No files matched the given criteria!");

	for (int m=0; m<=fileNameArray->GetMaxId(); m++)
	{
		fileName=(fileNameArray->GetValue(m));
		loadMetaImageFile(fileName);
		if (m_volumes)
		{
			vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
			image->DeepCopy(getConnector()->GetVTKImage());
			m_volumes->push_back(image);
		}
		if (m_fileNames_volstack)
			m_fileNames_volstack->push_back(fileName);

		int progress = (fileNameArray->GetMaxId() == 0) ? 100 : (m * 100) / fileNameArray->GetMaxId();
		observerProgress->manualProgressUpdate(progress);
	}
	addMsg(tr("%1  Loading volume stack completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	iosettingswriter();
}


void iAIO::readVolumeStack()
{
	for (int m=0; m<=fileNameArray->GetMaxId(); m++)
	{
		fileName=(fileNameArray->GetValue(m));
		readRawImage();
		vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
		image->DeepCopy(getConnector()->GetVTKImage());
		if(m_volumes)
			m_volumes->push_back(image);
		if(m_fileNames_volstack)
			m_fileNames_volstack->push_back(fileName);
		int progress = (m * 100) / fileNameArray->GetMaxId();
		observerProgress->manualProgressUpdate(progress);
	}
	addMsg(tr("%1  Loading volume stack completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	iosettingswriter();
}


void iAIO::writeVolumeStack()
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
		writeMetaImage(m_volumes->at(m).GetPointer(), fileNameArray->GetValue(m).c_str());
		int progress = (m * 100) / fileNameArray->GetMaxId();
		observerProgress->manualProgressUpdate(progress);
	}
}


void iAIO::readRawImage()
{
	VTK_TYPED_CALL(read_raw_image_template, scalarType, headersize, byteOrder,
		extent, spacing, origin, fileName, getItkProgress(), getConnector());
}


void iAIO::postImageReadActions()
{
	getVtkImageData()->ReleaseData();
	getVtkImageData()->Initialize();
	getVtkImageData()->DeepCopy(getConnector()->GetVTKImage());
	getVtkImageData()->CopyInformationFromPipeline(getConnector()->GetVTKImage()->GetInformation());
	addMsg(tr("%1  Loading file %2 completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(fileName));
}


void iAIO::readImageData()
{
	readRawImage();
	postImageReadActions();
	iosettingswriter();
}


void iAIO::readMetaImage( )
{
	loadMetaImageFile(fileName);
	postImageReadActions();
}


void iAIO::readSTL( )
{
	auto stlReader = vtkSmartPointer<vtkSTLReader>::New();
	stlReader->AddObserver(vtkCommand::ProgressEvent, observerProgress);
	stlReader->SetFileName(fileName.toLatin1());
	stlReader->SetOutput(getVtkPolyData());
	stlReader->Update();
	printSTLFileInfos();
	addMsg(tr("%1  Loading file %2 completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(fileName));
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

	dlg_commoninput dlg(parent, "Set file parameters", inList, inPara, NULL);

	if (dlg.exec() != QDialog::Accepted)
		return false;

	fileNamesBase = dlg.getText(0);
	extension = dlg.getText(1);
	digitsInIndex = dlg.getDblValue(2);
	indexRange[0] = dlg.getDblValue(3); indexRange[1]= dlg.getDblValue(4);
	FillFileNameArray(indexRange, digitsInIndex);
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
	QStringList datatype(VTKDataTypeList());
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

	dlg_openfile_sizecheck *dlg = new dlg_openfile_sizecheck (true, parent, "RAW file specs", inList, inPara, NULL, f);
	if (dlg->exec() == QDialog::Accepted){

		rawSizeX = dlg->getDblValue(5); rawSizeY = dlg->getDblValue(6); rawSizeZ = dlg->getDblValue(7);
		extent[0] = 0; extent[2] = 0; extent[4] = 0;
		extent[1] = rawSizeX; extent[3]= rawSizeY; extent[5] = rawSizeZ;
		extent[1]--; extent[3]--; extent[5]--;

		fileNamesBase = dlg->getText(0);
		extension = dlg->getText(1);
		digitsInIndex = dlg->getDblValue(2);
		indexRange[0] = dlg->getDblValue(3); indexRange[1]= dlg->getDblValue(4);
		spacing[0] = dlg->getDblValue(8); spacing[1]= dlg->getDblValue(9); spacing[2] = dlg->getDblValue(10);
		origin[0] = dlg->getDblValue(11); origin[1]= dlg->getDblValue(12); origin[2] = dlg->getDblValue(13);

		rawHeaderSize = dlg->getDblValue(15);
		headersize = rawHeaderSize;
		scalarType = MapVTKTypeStringToInt(dlg->getComboBoxValue(14));
		rawScalarType = scalarType;

		if (dlg->getComboBoxValue(16) == "Little Endian")
		byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
		else if (dlg->getComboBoxValue(16) == "Big Endian")
		byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;

		rawByteOrder = byteOrder;

		FillFileNameArray(indexRange, digitsInIndex);
	}
	else return false;

	return true;
}


bool iAIO::setupRAWReader( QString f )
{
	QStringList datatype(VTKDataTypeList());
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

	dlg_openfile_sizecheck *dlg = new dlg_openfile_sizecheck (false, parent, "RAW file specs", inList, inPara, NULL, f);

	if (dlg->exec() == QDialog::Accepted)
	{
		rawSizeX = dlg->getDblValue(0); rawSizeY = dlg->getDblValue(1); rawSizeZ = dlg->getDblValue(2);
		extent[0] = 0; extent[2] = 0; extent[4] = 0;
		extent[1] = rawSizeX; extent[3]= rawSizeY; extent[5] = rawSizeZ;
		extent[1]--; extent[3]--; extent[5]--;

		rawSpaceX = dlg->getDblValue(3); rawSpaceY = dlg->getDblValue(4); rawSpaceZ = dlg->getDblValue(5);
		spacing[0] = rawSpaceX; spacing[1]= rawSpaceY; spacing[2] = rawSpaceZ;

		rawOriginX = dlg->getDblValue(6); rawOriginY = dlg->getDblValue(7); rawOriginZ = dlg->getDblValue(8);
		origin[0] = rawOriginX; origin[1]= rawOriginY; origin[2] = rawOriginZ;

		rawHeaderSize = dlg->getDblValue(9);
		headersize = rawHeaderSize;
		fileName = f;
		scalarType = MapVTKTypeStringToInt(dlg->getComboBoxValue(10));
		rawScalarType = scalarType;
		if (dlg->getComboBoxValue(11) == "Little Endian")
			byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
		else if (dlg->getComboBoxValue(11) == "Big Endian")
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


void iAIO::writeMetaImage( vtkSmartPointer<vtkImageData> imgToWrite, QString fileName )
{
	iAConnector con; con.SetImage(imgToWrite); con.Modified();
	iAConnector::ITKScalarPixelType itkType = con.GetITKScalarPixelType();
	iAConnector::ITKPixelType itkPixelType = con.GetITKPixelType();
	ITK_EXTENDED_TYPED_CALL(write_image_template, itkType, itkPixelType,
		compression, fileName, getItkProgress(), &con);
	addMsg(tr("%1  Saved as file '%2'.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(fileName));
}


void iAIO::writeSTL( )
{
	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	stlWriter->AddObserver(vtkCommand::ProgressEvent, observerProgress);
	stlWriter->SetFileName(fileName.toLatin1());
	stlWriter->SetInputData(getVtkPolyData());
	stlWriter->SetFileTypeToBinary();
	stlWriter->Write();
	addMsg(tr("%1  Saved as file '%2'.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(fileName));
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


void iAIO::writeImageStack( )
{
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType PixelType;
	getConnector()->SetImage(getVtkImageData());
	const ScalarPixelType pixelType = getConnector()->GetITKScalarPixelType();
	const PixelType imagePixelType = getConnector()->GetITKPixelType();
	ITK_EXTENDED_TYPED_CALL(writeImageStack_template, pixelType, imagePixelType,
		fileName, getItkProgress(), getConnector(), compression);
	addMsg(tr("%1  %2 Image Stack saved.")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(QFileInfo(fileName).completeSuffix().toUpper()));
	addMsg("  Base Filename: " + fileName);
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
		<< tr("%1").arg(origin[0]) << tr("%1").arg(origin[1]) << tr("%1").arg(origin[2]) << VTKDataTypeList());

	dlg_commoninput dlg(parent, "Set file parameters", inList, inPara, NULL);

	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	fileNamesBase = dlg.getText(0);
	extension = dlg.getText(1);
	digitsInIndex = dlg.getDblValue(2);
	indexRange[0] = dlg.getDblValue(3); indexRange[1]= dlg.getDblValue(4);
	spacing[0] = dlg.getDblValue(5); spacing[1]= dlg.getDblValue(6); spacing[2] = dlg.getDblValue(7);
	origin[0] = dlg.getDblValue(8); origin[1]= dlg.getDblValue(9); origin[2] = dlg.getDblValue(10);
	scalarType = MapVTKTypeStringToInt(dlg.getComboBoxValue(11));
	FillFileNameArray(indexRange, digitsInIndex);
	return true;
}


void iAIO::readImageStack()
{
	vtkSmartPointer<vtkImageReader2> imgReader;
	switch (ioID)
	{
		case TIF_STACK_READER: imgReader = vtkSmartPointer<vtkTIFFReader>::New(); break;
		case JPG_STACK_READER: imgReader = vtkSmartPointer<vtkJPEGReader>::New(); break;
		case PNG_STACK_READER: imgReader = vtkSmartPointer<vtkPNGReader>::New(); break;
		case BMP_STACK_READER: imgReader = vtkSmartPointer<vtkBMPReader>::New(); break;
		default: throw std::runtime_error("Invalid Image Stack IO id, aborting.");
	}
	imgReader->ReleaseDataFlagOn();
	imgReader->AddObserver(vtkCommand::ProgressEvent, observerProgress);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	imgReader->SetFileNames(fileNameArray);
	imgReader->SetDataOrigin(origin);
	imgReader->SetDataSpacing(spacing);
	imgReader->SetOutput(getVtkImageData());
	imgReader->Update();
	addMsg(tr("%1  Loading image stack completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
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
	addMsg(tr("  Cells: %1").arg(getVtkPolyData()->GetNumberOfCells()));
	addMsg(tr("  Points: %1").arg(getVtkPolyData()->GetNumberOfPoints()));
	addMsg(tr("  Polygons: %1").arg(getVtkPolyData()->GetNumberOfPolys()));
	addMsg(tr("  Lines: %1").arg(getVtkPolyData()->GetNumberOfLines()));
	addMsg(tr("  Strips: %1").arg(getVtkPolyData()->GetNumberOfStrips()));
	addMsg(tr("  Pieces: %1").arg(getVtkPolyData()->GetNumberOfPieces()));
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
