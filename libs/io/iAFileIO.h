// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include <iAAttributes.h>
#include <iADataSet.h>
#include <iADataSetType.h>
#include <iAProgress.h>

#include <QMap>
#include <QString>

#include <memory>


//! Base class for dataset readers/writers within the file I/O framework of open_iA.
//! Derived classes (for loading/saving specific file types) should be registered via iAFileTypeRegistry;
//! Automatic registration works by using iAAutoRegistration.
class iAio_API iAFileIO
{
public:
	enum Operation
	{
		Load,
		Save
	};
	static const QString CompressionStr;
	//! create a file I/O for the given dataset type
	iAFileIO(iADataSetTypes readTypes, iADataSetTypes writeTypes);
	//! virtual destructor, to enable proper destruction in derived classes and to avoid warnings
	virtual ~iAFileIO();
	//! The name of the file type that this IO supports
	virtual QString name() const = 0;
	//! The file extensions that this file IO should be used for
	virtual QStringList extensions() const = 0;

	//! Required parameters for loading/saving the file
	iAAttributes const& parameter(Operation op) const;
	//! Types of dataset contained in this file format, which this IO can load/save
	iADataSetTypes supportedDataSetTypes(Operation op) const;

	//! Load the (list of) dataset(s); store parameters in the resulting datasets
	std::shared_ptr<iADataSet> load(QString const& fileName, QVariantMap const& paramValues, iAProgress const & progress = iAProgress());

	//! Whether this IO can be used for storing the given data set.
	//! It could for example check whether the format supports the data types in the dataset
	//! The default implementation here always returns true
	virtual bool isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const;
	//! Save the (list of) dataset(s); modify input datasets to reflect the new file name this data is now stored under
	void save(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress = iAProgress());
	//! Check whether the given values contain all required parameters; set to default if not
	bool checkParams(QVariantMap & paramValues, Operation op, QString const& fileName);

	//! list of extensions as required by Qt's open/save file dialogs
	QStringList filterExtensions();
	//! a filter string for the type of files supported by the I/O class
	QString filterString();

protected:

	std::array<iAAttributes, 2> m_params;

	//! I/O for specific file formats should override this to load data from the file with given name. default implementation does nothing
	//! (instead of being pure virtual, to allow for I/O's that only save a dataset but don't load one)
	virtual std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress);
	//! I/O for specific file formats should override this to save data to the file with given name. default implementation does nothing
	//! (instead of being pure virtual, to allow for I/O's that only load a dataset but don't save one).
	//! The file name and all Save parameter values (m_params[Save]) will be set in save()
	//! Derived classes must add any potentially necessary metadata in the dataSets so that there is all information in there to load the dataSet again from the given fileName
	virtual void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress);

private:
	std::array<iADataSetTypes, 2> m_dataSetTypes;
};
