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
#include "iADataSet.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType

#include <vtkImageData.h>
#include <vtkPolyData.h>

#include <QFileInfo>

#include <array>

QString boundsStr(double const* bds)
{
	return QString("Bounds: x=%1..%2, y=%3..%4, z=%5..%6")
		.arg(bds[0]).arg(bds[1])
		.arg(bds[2]).arg(bds[3])
		.arg(bds[4]).arg(bds[5]);
}

namespace
{
	QString meshInfo(vtkPolyData* mesh)
	{
		return QString("Points: %1; Lines: %2; Cells: %3\n")
			.arg(mesh->GetNumberOfPoints()).arg(mesh->GetNumberOfLines()).arg(mesh->GetNumberOfCells()) +
			QString("Polygons: %1; Strips: %2; Pieces: %3\n%4")
			.arg(mesh->GetNumberOfPolys()).arg(mesh->GetNumberOfStrips()).arg(mesh->GetNumberOfPieces()).arg(boundsStr(mesh->GetBounds()));
	}}

iADataSet::iADataSet(QString const& fileName, iADataSetType type, QString const& name) :
	m_fileName(fileName),
	m_name(name.isEmpty()? QFileInfo(fileName).baseName() : name),
	m_type(type)
{
}

iADataSet::~iADataSet()
{}

QString const& iADataSet::name() const
{
	return m_name;
}

void iADataSet::setName(QString const& newName)
{
	m_name = newName;
}

QString const& iADataSet::fileName() const
{
	return m_fileName;
}

std::array<double, 3> iADataSet::unitDistance() const
{
	return { 1.0, 1.0, 1.0 };
}

QString iADataSet::info() const
{
	return "";
}

void iADataSet::setParameters(QVariantMap const& parameters)
{
	m_parameters = parameters;
}

QVariantMap const& iADataSet::parameters() const
{
	return m_parameters;
}

iADataSetType iADataSet::type() const
{
	return m_type;
}

// ---------- iAPolyData ----------

iAPolyData::iAPolyData(QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(fileName, iADataSetType::Mesh),
	m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAPolyData::poly()
{
	return m_mesh;
}

QString iAPolyData::info() const
{
	return meshInfo(m_mesh);
}

std::array<double, 3> iAPolyData::unitDistance() const
{
	auto bounds = m_mesh->GetBounds();
	const double Divisor = 100;
	return {
		(bounds[1] - bounds[0]) / Divisor,
		(bounds[3] - bounds[2]) / Divisor,
		(bounds[5] - bounds[4]) / Divisor
	};
}

// ---------- iAGraphData ----------

iAGraphData::iAGraphData(QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(fileName, iADataSetType::Graph), m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAGraphData::poly()
{
	return m_mesh;
}

QString iAGraphData::info() const
{
	return meshInfo(m_mesh);
}

// ---------- iAImageData ----------

iAImageData::iAImageData(QString const& fileName, vtkSmartPointer<vtkImageData> img):
	iADataSet(fileName, iADataSetType::Volume),
	m_img(img)
{
}

vtkSmartPointer<vtkImageData> iAImageData::image()
{
	return m_img;
}

unsigned long long iAImageData::voxelCount() const
{
	auto const ext = m_img->GetExtent();
	return static_cast<unsigned long long>(ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
}

QString iAImageData::info() const
{
	auto const ext = m_img->GetExtent();
	auto const spc = m_img->GetSpacing();
	auto const ori = m_img->GetOrigin();
	return
		QString("Extent: x=%1..%2; y=%3..%4 z=%5..%6 (%7 voxels)\n")
			.arg(ext[0]).arg(ext[1])
			.arg(ext[2]).arg(ext[3])
			.arg(ext[4]).arg(ext[5])
			.arg(voxelCount()) +
		QString("Origin: %1 %2 %3; Spacing: %4 %5 %6; Components: %7\n")
			.arg(ori[0]).arg(ori[1]).arg(ori[2])
			.arg(spc[0]).arg(spc[1]).arg(spc[2])
			.arg(m_img->GetNumberOfScalarComponents()) +
		QString("Data type: %1\n").arg(mapVTKTypeToReadableDataType(m_img->GetScalarType()));
	/*
	if (m_img->GetNumberOfScalarComponents() == 1)  //No histogram statistics for rgb, rgba or vector pixel type images
	{
		if (info.isComputing())
		{
			lWidget->addItem("    Statistics are currently computing...");
		}
		else if (info.voxelCount() == 0)
		{
			lWidget->addItem("    Statistics not computed yet. Activate modality (by clicking on it) to do so.");
		}
		else
		{
			lWidget->addItem(tr("    VoxelCount: %1;  Min: %2;  Max: %3;  Mean: %4;  StdDev: %5;")
								 .arg(info.voxelCount())
								 .arg(info.min())
								 .arg(info.max())
								 .arg(info.mean())
								 .arg(info.standardDeviation()));
		}
	}
	*/
}

std::array<double, 3> iAImageData::unitDistance() const
{
	auto const spc = m_img->GetSpacing();
	return { spc[0],  spc[1], spc[2] };
}
