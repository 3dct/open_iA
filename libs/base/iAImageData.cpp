// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageData.h"

#include "iAConnector.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType

#include <vtkImageData.h>

#include <QLocale>

iAImageData::iAImageData(vtkSmartPointer<vtkImageData> img) :
	iADataSet(iADataSetType::Volume),
	m_img(img),
	m_con(nullptr)
{}

iAImageData::iAImageData(itk::ImageBase<3>* itkImg) :
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
		.arg(QLocale().toString(voxelCount())) +
		QString("Origin: %1 %2 %3; Spacing: %4 %5 %6; Components: %7\n")
		.arg(ori[0]).arg(ori[1]).arg(ori[2])
		.arg(spc[0]).arg(spc[1]).arg(spc[2])
		.arg(m_img->GetNumberOfScalarComponents()) +
		QString("Data type: %1\n").arg(mapVTKTypeToReadableDataType(m_img->GetScalarType()));
}

std::array<double, 3> iAImageData::unitDistance() const
{
	auto const spc = m_img->GetSpacing();
	return { spc[0],  spc[1], spc[2] };
}
