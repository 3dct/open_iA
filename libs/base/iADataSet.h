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

#include "iALog.h"

#include <vtkSmartPointer.h>

#include <QFlags>
#include <QString>

class iADataSet;
class iAProgress;

class vtkPolyData;
class vtkImageData;

//! abstract interface for datasets
class iAbase_API iADataSet
{
public:
	//iAVec3d bounds();
	//virtual double const* bounds() const = 0;
	//virtual QString information() const =0;
	
	//! The name of the dataset (defaults to the "basename" of the file)
	QString const& name() const;
	//! The name of the file in which this dataset is stored
	QString const& fileName() const;
	//! a sensible unit distance for this dataset (e.g. the spacing of a single voxel, for volume datasets)
	virtual std::array<double, 3> unitDistance() const;
	//! should deliver information about the dataset interesting to users viewing it
	virtual QString info() const;

protected:
	iADataSet(QString const& name, QString const& fileName);

private:
	QString m_name;
	QString m_fileName;
};

class iAbase_API iAPolyData : public iADataSet
{
public:
	iAPolyData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly();
	QString info() const override;
	std::array<double, 3> unitDistance() const override;

private:
	vtkSmartPointer<vtkPolyData> m_mesh;
};

class iAbase_API iAGraphData : public iADataSet
{
public:
	iAGraphData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly();
	QString info() const override;

private:
	vtkSmartPointer<vtkPolyData> m_mesh;
};

class iAbase_API iAImageData : public iADataSet
{
public:
	iAImageData(QString const& name, QString const& fileName, vtkSmartPointer<vtkImageData> img);
	vtkSmartPointer<vtkImageData> image();
	QString info() const override;
	std::array<double, 3> unitDistance() const override;

private:
	vtkSmartPointer<vtkImageData> m_img;
};
