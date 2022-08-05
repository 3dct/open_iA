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

namespace
{
	QString meshInfo(vtkPolyData* mesh)
	{
		return QString("Points: %1; Lines: %2; Cells: %3\n")
			.arg(mesh->GetNumberOfPoints()).arg(mesh->GetNumberOfLines()).arg(mesh->GetNumberOfCells()) +
			QString("Polygons: %1; Strips: %2; Pieces: %3")
			.arg(mesh->GetNumberOfPolys()).arg(mesh->GetNumberOfStrips()).arg(mesh->GetNumberOfPieces());
	}}

iADataSet::iADataSet(QString const& name, QString const& fileName) :
	m_name(name), m_fileName(fileName)
{
}

QString const& iADataSet::name() const
{
	return m_name;
}

QString const& iADataSet::fileName() const
{
	return m_fileName;
}

QString iADataSet::info() const
{
	return "";
}

iAPolyData::iAPolyData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(name, fileName),
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



iAGraphData::iAGraphData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(name, fileName), m_mesh(mesh)
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



iAImageData::iAImageData(QString const& name, QString const& fileName, vtkSmartPointer<vtkImageData> img):
	iADataSet(name, fileName),
	m_img(img)
{
}

vtkSmartPointer<vtkImageData> iAImageData::image()
{
	return m_img;
}

QString iAImageData::info() const
{
	return
		QString("Extent (pixel): x=%1..%2; y=%3..%4 z=%5..%6\n")
			.arg(m_img->GetExtent()[0]).arg(m_img->GetExtent()[1])
			.arg(m_img->GetExtent()[2]).arg(m_img->GetExtent()[3])
			.arg(m_img->GetExtent()[4]).arg(m_img->GetExtent()[5]) +
		QString("Origin: %1 %2 %3; Spacing: %4 %5 %6\n")
			.arg(m_img->GetOrigin()[0]).arg(m_img->GetOrigin()[1]).arg(m_img->GetOrigin()[2])
			.arg(m_img->GetSpacing()[0]).arg(m_img->GetSpacing()[1]).arg(m_img->GetSpacing()[2]) +
		QString("Data type: %1\n").arg(mapVTKTypeToReadableDataType(m_img->GetScalarType())) +
		QString("Components: %1").arg(m_img->GetNumberOfScalarComponents());
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
