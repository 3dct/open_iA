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

//! abstract interface for datasets
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
	//! retrieve (optional) additional parameters for the dataset
	QVariant metaData(QString const& key) const;
	//! true if the dataset has metadata with the given key set, false otherwise
	bool hasMetaData(QString const& key) const;

	//! get type of data stored in this dataset
	iADataSetType type() const;

protected:
	//! derived classes need to construct the dataset by giving a (proposed) filename and an (optional) name
	iADataSet(iADataSetType type);

private:
	//! @{ prevent copying
	iADataSet(iADataSet const& other) = delete;
	iADataSet& operator=(iADataSet const& other) = delete;
	//! @}
	iADataSetType m_type;      //!< type of data in this dataset
	QVariantMap m_metaData;    //!< (optional) additional metadata that is required to load the file, or that came along with the dataset
};

//! a polygon (surface) mesh
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
class iAbase_API iAGraphData : public iADataSet
{
public:
	iAGraphData(vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly() const;
	QString info() const override;

private:
	iAGraphData(iAGraphData const& other) = delete;
	iAGraphData& operator=(iAGraphData const& other) = delete;
	vtkSmartPointer<vtkPolyData> m_mesh;
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

iAbase_API QString boundsStr(double const* bds);