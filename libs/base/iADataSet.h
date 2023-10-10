// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include "iADataSetType.h"

#include <QString>
#include <QVariant>    // for QVariantMap (at least under Qt 5.15.2)

//! Abstract interface for datasets.
class iAbase_API iADataSet
{
public:
	//! called when the dataset is removed/unloaded and its related resources should be released
	virtual ~iADataSet();

	static const QString NameKey;         //!< metadata key for name of the dataset
	static const QString FileNameKey;     //!< metadata key for filename of the dataset
	//! convenience method for accessing value for NameKey in m_metaData
	QString name() const;
	//! a sensible unit distance for this dataset (e.g. the spacing of a single voxel, for volume datasets)
	virtual std::array<double, 3> unitDistance() const;
	//! should deliver information about the dataset interesting to users viewing it; implemented by derived classes
	virtual QString info() const;

	//! set an (optional) metadata key/value pair
	//! @param key the key to be added; if it already exists, its value is overwritten
	//! @param value the value for the given key
	void setMetaData(QString const & key, QVariant const& value);
	//! Set bulk metadata from another key/value map.
	//! This adds/overwrites the key/value pairs given via parameter; it does NOT delete keys in the dataset's metadata which don't exist in the given parameter
	//! @param other map of meta data; all key-value pairs in this list are added to this dataset's metadata (for already existing keys, the values are overwritten)
	void setMetaData(QVariantMap const& other);
	//! retrieve (optional) additional parameters for the dataset
	QVariant metaData(QString const& key) const;
	//! true if the dataset has metadata with the given key set, false otherwise
	bool hasMetaData(QString const& key) const;
	//! return the key, value map of all metadata items associated with the dataset
	QVariantMap const& allMetaData() const;

	//! get type of data stored in this dataset
	iADataSetType type() const;

protected:
	//! derived classes need to construct the dataset by giving a (proposed) filename and an (optional) name
	iADataSet(iADataSetType type);

private:
	Q_DISABLE_COPY_MOVE(iADataSet);
	iADataSetType m_type;      //!< type of data in this dataset
	QVariantMap m_metaData;    //!< (optional) additional metadata that is required to load the file, or that came along with the dataset
};

class QSettings;

//! a collection of datasets
class iAbase_API iADataCollection : public iADataSet
{
public:
	iADataCollection(size_t capacity, std::shared_ptr<QSettings> settings);
	std::vector<std::shared_ptr<iADataSet>> const & dataSets() const;
	void addDataSet(std::shared_ptr<iADataSet> dataSet);
	QString info() const override;
	std::shared_ptr<QSettings> settings() const;

private:
	iADataCollection(iADataCollection const& other) = delete;
	iADataCollection& operator=(iADataCollection const& other) = delete;
	std::vector<std::shared_ptr<iADataSet>> m_dataSets;
	std::shared_ptr<QSettings> m_settings;
};

iAbase_API QString boundsStr(double const* bds);
