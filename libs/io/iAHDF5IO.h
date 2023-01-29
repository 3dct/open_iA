// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef USE_HDF5

#include "iAio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

// for now, let's use HDF5 1.10 API:
#define H5_USE_110_API
#include <hdf5.h>

class iAio_API iAHDF5IO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAHDF5IO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	static const QString DataSetPathStr;
	static const QString SpacingStr;

	iAHDF5IO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

iAio_API QString hdf5MapTypeToString(H5T_class_t hdf5Type);
iAio_API int hdf5GetNumericVTKTypeFromHDF5Type(H5T_class_t hdf5Type, size_t numBytes, H5T_sign_t sign);
iAio_API void hdf5PrintErrorsToConsole();
iAio_API bool hdf5IsITKImage(hid_t file_id);

#endif
