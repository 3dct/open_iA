// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADataSet.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImageBase.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkSmartPointer.h>

class iAConnector;

class vtkImageData;

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
