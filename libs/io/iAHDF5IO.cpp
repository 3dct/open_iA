// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef USE_HDF5

#include "iAHDF5IO.h"

#include "iAImageData.h"
#include "iAToolsITK.h"    // for storeImage, pulls in iAITKIO
#include "iAValueTypeVectorHelpers.h"

#include <vtkImageData.h>
#include <vtkImageImport.h>

namespace
{
	const int InvalidHDF5Type = -1;

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

	herr_t errorfunc(unsigned /*n*/, const H5E_error2_t* err, void* /*client_data*/)
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

	bool hdf5GroupExists(hid_t file_id, const char* name)
	{
		hid_t loc_id = H5Gopen(file_id, name, H5P_DEFAULT);
		bool result = loc_id > 0;
		if (result)
		{
			H5Gclose(loc_id);
		}
		return result;
	}

	bool hdf5DatasetExists(hid_t file_id, const char* name)
	{
		hid_t loc_id = H5Dopen(file_id, name, H5P_DEFAULT);
		bool result = loc_id > 0;
		if (result)
		{
			H5Dclose(loc_id);
		}
		return result;
	}
}

const QString iAHDF5IO::Name("HDF5 file");
const QString iAHDF5IO::DataSetPathStr("Dataset path");
const QString iAHDF5IO::SpacingStr("Spacing");

iAHDF5IO::iAHDF5IO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{
	addAttr(m_params[Load], DataSetPathStr, iAValueType::String, "");
	addAttr(m_params[Load], SpacingStr, iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
}

std::shared_ptr<iADataSet> iAHDF5IO::loadData(QString const& fileName, QVariantMap const& params, iAProgress const& progress)
{
	Q_UNUSED(progress);
	hid_t file_id = H5Fopen(fileName.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (hdf5IsITKImage(file_id))
	{
		H5Fclose(file_id);
		iAITKIO::PixelType pixelType;
		iAITKIO::ScalarType scalarType;
		auto img = iAITKIO::readFile(fileName, pixelType, scalarType, true);
		return { std::make_shared<iAImageData>(img) };
	}
	auto hdf5PathStr = params[DataSetPathStr].toString();
	auto hdf5Path = hdf5PathStr.split("/");
	if (hdf5Path.size() < 1)
	{
		H5Fclose(file_id);
		throw std::runtime_error(QString("HDF5 file %1: At least one path element expected, 0 given.").arg(fileName).toStdString());
	}
	hid_t loc_id = file_id;
	auto dataSetName = hdf5Path.takeLast();
	if (!hdf5Path.isEmpty())
	{
		auto groupName = hdf5Path.join("/");
		loc_id = H5Gopen(file_id, groupName.toStdString().c_str(), H5P_DEFAULT);  // TODO: check which encoding HDF5 internal strings have!
		if (loc_id < 0)
		{
			H5Fclose(file_id);
			throw std::runtime_error(QString("HDF5 file %1: Could not open group %2.").arg(fileName).arg(groupName).toStdString());
		}
	}
	hid_t dataset_id = H5Dopen(loc_id, dataSetName.toStdString().c_str(), H5P_DEFAULT);
	hid_t space = H5Dget_space(dataset_id);
	int rank = H5Sget_simple_extent_ndims(space);
	if (rank < 0)
	{
		H5Fclose(file_id);
		throw std::runtime_error(QString("HDF5 file %1: Retrieving rank of dataset %2 failed.").arg(fileName).arg(dataSetName).toStdString());
	}
	hsize_t* hdf5Dims = new hsize_t[rank];
	hsize_t* maxdims = new hsize_t[rank];
	int status = H5Sget_simple_extent_dims(space, hdf5Dims, maxdims);
	hid_t type_id = H5Dget_type(dataset_id);
	H5T_class_t hdf5Type = H5Tget_class(type_id);
	size_t numBytes = H5Tget_size(type_id);
	H5T_sign_t sign = H5Tget_sign(type_id);
	int vtkType = hdf5GetNumericVTKTypeFromHDF5Type(hdf5Type, numBytes, sign);
	H5Tclose(type_id);
	status = H5Sclose(space);
	if (vtkType == InvalidHDF5Type)
	{
		H5Fclose(file_id);
		throw std::runtime_error(QString("HDF5 file %1: Can't load a dataset of data type %2!").arg(fileName).arg(hdf5Type).toStdString());
	}
	int dim[3];
	for (int i = 0; i < 3; ++i)
	{
		if (i < rank && hdf5Dims[i] > static_cast<hsize_t>(std::numeric_limits<int>::max()))
		{
			throw std::runtime_error(QString("HDF5 file %1: Dataset size %2 in dimension %3 is larger than what VTK image datasets can handle!").arg(fileName).arg(hdf5Dims[i]).arg(i).toStdString());
		}
		dim[i] = (i < rank) ? static_cast<int>(hdf5Dims[i]) : 1;
	}
	unsigned char* raw_data = new unsigned char[numBytes * dim[0] * dim[1] * dim[2]];
	status = H5Dread(dataset_id, GetHDF5ReadType(hdf5Type, numBytes, sign), H5S_ALL, H5S_ALL, H5P_DEFAULT, raw_data);
	if (status < 0)
	{
		hdf5PrintErrorsToConsole();
		H5Fclose(file_id);
		throw std::runtime_error(QString("HDF5 file %1: Reading dataset failed!").arg(fileName).toStdString());
	}
	H5Dclose(dataset_id);
	if (!hdf5Path.isEmpty())
	{
		H5Gclose(loc_id);
	}
	H5Fclose(file_id);

	vtkNew<vtkImageImport> imgImport;
	auto spc = variantToVector<double>(params[SpacingStr]);
	imgImport->SetDataSpacing(spc[2], spc[1], spc[0]);
	imgImport->SetDataOrigin(0, 0, 0);
	imgImport->SetWholeExtent(0, dim[2] - 1, 0, dim[1] - 1, 0, dim[0] - 1);
	imgImport->SetDataExtentToWholeExtent();
	imgImport->SetDataScalarType(vtkType);
	imgImport->SetNumberOfScalarComponents(1);
	imgImport->SetImportVoidPointer(raw_data, 0);
	//imgImport->ReleaseDataFlagOff(); // doesn't change anything about below situation
	imgImport->Update();
	//auto img = imgImport->GetOutput();  // < does not work, as imgImport cleans up after himself
	// according to https://vtk.org/Wiki/VTK/Tutorials/SmartPointers, one should be able to do:
	//vtkSmartPointer<vtkImageData> img = imgImport->GetOutput();
	// but it causes the same errors as the code above; so we need to deep-copy:
	auto img = vtkSmartPointer<vtkImageData>::New();
	img->DeepCopy(imgImport->GetOutput());
	auto ds = std::make_shared<iAImageData>(img);
	ds->setMetaData(params);
	return ds;
}

void iAHDF5IO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	// trust ITK default logic to find usable IO:
	storeImage(dynamic_cast<iAImageData*>(dataSet.get())->itkImage(), fileName, false, &progress);
}

QString iAHDF5IO::name() const
{
	return iAHDF5IO::Name;
}

QStringList iAHDF5IO::extensions() const
{
	return QStringList{ "hdf", "hdf5", "h5", "he5", "nc", "cdf", "mat" };
}


QString hdf5MapTypeToString(H5T_class_t hdf5Type)
{
	switch (hdf5Type)
	{
	case H5T_NO_CLASS: return QString("No Class");
	case H5T_INTEGER: return QString("Integer");
	case H5T_FLOAT: return QString("Float");
	case H5T_TIME: return QString("Time");
	case H5T_STRING: return QString("String");
	case H5T_BITFIELD: return QString("Bitfield");
	case H5T_OPAQUE: return QString("Opaque");
	case H5T_COMPOUND: return QString("Compound");
	case H5T_REFERENCE: return QString("Reference");
	case H5T_ENUM: return QString("Enum");
	case H5T_VLEN: return QString("VLen");
	case H5T_ARRAY: return QString("Array");
	default: return QString("Unknown");
	}
}

int hdf5GetNumericVTKTypeFromHDF5Type(H5T_class_t hdf5Type, size_t numBytes, H5T_sign_t sign)
{
	switch (hdf5Type)
	{
	case H5T_INTEGER: {
		switch (numBytes)
		{
		case 1: return (sign == H5T_SGN_NONE) ? VTK_UNSIGNED_CHAR : VTK_SIGNED_CHAR;
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

void hdf5PrintErrorsToConsole()
{
	hid_t err_stack = H5Eget_current_stack();
	/*herr_t walkresult = */ H5Ewalk(err_stack, H5E_WALK_UPWARD, errorfunc, nullptr);
}

bool hdf5IsITKImage(hid_t file_id)
{
	return hdf5GroupExists(file_id, "ITKImage") && hdf5DatasetExists(file_id, "ITKVersion");
}

#endif
