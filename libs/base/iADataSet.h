// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include "iADataSetType.h"
#include "iALog.h"

#include <vtkSmartPointer.h>

#include <QFlags>
#include <QMap>
#include <QString>
#include <QVariant>    // for QVariantMap (at least under Qt 5.15.2)

class iAConnector;
class iAProgress;

class vtkPolyData;
class vtkImageData;

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
	//! should deliver information about the dataset interesting to users viewing it
	virtual QString info() const;

	//! set an (optional) metadata key/value pair
	void setMetaData(QString const & key, QVariant const& value);
	//! set bulk metadata from another key/value map
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

//! a class for vtk polydata mesh datasets
class iAbase_API iAPolyData : public iADataSet
{
public:
	iAPolyData(vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly() const;
	QString info() const override;
	std::array<double, 3> unitDistance() const override;

private:
	iAPolyData(iAPolyData const & other) = delete;
	iAPolyData& operator=(iAPolyData const& other) = delete;
	vtkSmartPointer<vtkPolyData> m_mesh;
};

//! a graph dataset
//! merge with iAPolyData ?
class iAbase_API iAGraphData : public iADataSet
{
public:
	iAGraphData(vtkSmartPointer<vtkPolyData> mesh,
		QStringList const & vertexValueNames,
		QStringList const & edgeValueNames);
	vtkSmartPointer<vtkPolyData> poly() const;
	QString info() const override;
	//std::array<double, 3> unitDistance() const override;
	QStringList const & vertexValueNames() const;
	QStringList const& edgeValueNames() const;

private:
	iAGraphData(iAGraphData const& other) = delete;
	iAGraphData& operator=(iAGraphData const& other) = delete;

	vtkSmartPointer<vtkPolyData> m_mesh;
	QStringList m_vertexValueNames, m_edgeValueNames;
};

#include <itkImageBase.h>

//! an image (/volume) dataset
class iAbase_API iAImageData : public iADataSet
{
public:
	iAImageData(vtkSmartPointer<vtkImageData> img);
	iAImageData(itk::ImageBase<3>* itkImg);
	~iAImageData();
	vtkSmartPointer<vtkImageData> vtkImage() const;
	itk::ImageBase<3>* itkImage() const;
	QString info() const override;
	std::array<double, 3> unitDistance() const override;
	unsigned long long voxelCount() const;

private:
	iAImageData(iAImageData const& other) = delete;
	iAImageData& operator=(iAImageData const& other) = delete;
	vtkSmartPointer<vtkImageData> m_img;
	mutable iAConnector* m_con;
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
