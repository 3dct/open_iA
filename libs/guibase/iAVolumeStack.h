// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <vtkSmartPointer.h>

#include <QString>

#include <vector>

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;

// replace through view on a collection of image dataset viewers
class iAguibase_API iAVolumeStack
{
	public:
		~iAVolumeStack();

		vtkImageData* volume(size_t i);
		void addVolume(vtkImageData* volume);
		void addFileName(QString fileName);
		QString fileName(size_t i);
		size_t numberOfVolumes();
		void addVolumeAt(vtkImageData* volume, size_t i);
		void addColorTransferFunction(vtkColorTransferFunction* instance);
		void addPiecewiseFunction(vtkPiecewiseFunction* instance);
		vtkColorTransferFunction* colorTF(size_t i);
		vtkPiecewiseFunction* opacityTF(size_t i);
		std::vector<vtkSmartPointer<vtkImageData> > * volumes();
		std::vector<QString> * fileNames();

	private:
		std::vector<vtkSmartPointer<vtkImageData> > m_volumes;
		std::vector<vtkColorTransferFunction*> m_colorTFVector;
		std::vector<vtkPiecewiseFunction*> m_opacityTFVector;
		std::vector<QString> m_fileNameArray;
};
