// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QWidget>

class iASlicer;

class vtkImageData;
class vtkScalarsToColors;
class vtkTransform;

class iAImageWidget: public QWidget
{
public:
	iAImageWidget(vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkScalarsToColors> lut);
	void StyleChanged();
	void SetMode(int slicerMode);
	void SetSlice(int sliceNumber);
	int GetSliceCount() const;
	iASlicer* GetSlicer();
private:
	vtkSmartPointer<vtkTransform> m_transform;
	vtkSmartPointer<vtkScalarsToColors> m_lut;
	iASlicer* m_slicer;
};
