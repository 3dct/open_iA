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

const QString iADataSet::NameKey("Name");
const QString iADataSet::FileNameKey("File");

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

iADataSet::iADataSet(iADataSetType type) :
	m_type(type)
{}

iADataSet::~iADataSet()
{}

QString iADataSet::name() const
{
	return metaData(iADataSet::NameKey).toString();
}

std::array<double, 3> iADataSet::unitDistance() const
{
	return { 1.0, 1.0, 1.0 };
}

QString iADataSet::info() const
{
	return "";
}

void iADataSet::setMetaData(QString const& key, QVariant const& value)
{
	m_metaData[key] = value;
}

void iADataSet::setMetaData(QVariantMap const& other)
{
	m_metaData.insert(other);
}

QVariant iADataSet::metaData(QString const& key) const
{
	return m_metaData[key];
}

bool iADataSet::hasMetaData(QString const& key) const
{
	return m_metaData.contains(key);
}

QVariantMap const& iADataSet::allMetaData() const
{
	return m_metaData;
}

iADataSetType iADataSet::type() const
{
	return m_type;
}

// ---------- iAPolyData ----------

iAPolyData::iAPolyData(vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(iADataSetType::Mesh),
	m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAPolyData::poly() const
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

iAGraphData::iAGraphData(vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(iADataSetType::Graph), m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAGraphData::poly() const
{
	return m_mesh;
}

QString iAGraphData::info() const
{
	return meshInfo(m_mesh);
}

// ---------- iAImageData ----------

#include "iAConnector.h"

iAImageData::iAImageData(vtkSmartPointer<vtkImageData> img):
	iADataSet(iADataSetType::Volume),
	m_img(img),
	m_con(nullptr)
{}

iAImageData::iAImageData(itk::ImageBase<3>* itkImg):
	iADataSet(iADataSetType::Volume),
	m_img(vtkSmartPointer<vtkImageData>::New()),
	m_con(nullptr)
{
	iAConnector con;
	con.setImage(itkImg);
	m_img->DeepCopy(con.vtkImage());
}

iAImageData::~iAImageData()
{
	delete m_con;
}

vtkSmartPointer<vtkImageData> iAImageData::vtkImage() const
{
	return m_img;
}

itk::ImageBase<3>* iAImageData::itkImage() const
{
	if (!m_con)
	{
		m_con = new iAConnector();
	}
	m_con->setImage(m_img);
	return m_con->itkImage();
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


iADataCollection::iADataCollection(size_t capacity, std::shared_ptr<QSettings> settings):
	iADataSet(iADataSetType::Collection),
	m_settings(settings)
{
	m_dataSets.reserve(capacity);
}

std::vector<std::shared_ptr<iADataSet>> const & iADataCollection::dataSets() const
{
	return m_dataSets;
}

void iADataCollection::addDataSet(std::shared_ptr<iADataSet> dataSet)
{
	m_dataSets.push_back(dataSet);
}

QString iADataCollection::info() const
{
	return QString("Number of datasets: %1").arg(m_dataSets.size());
}

std::shared_ptr<QSettings> iADataCollection::settings() const
{
	return m_settings;
}
