// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QWidget>

class iASlicer;

class vtkImageData;
class vtkScalarsToColors;

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
	vtkSmartPointer<vtkScalarsToColors> m_lut;
	iASlicer* m_slicer;
};
