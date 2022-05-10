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

#include "iAguibase_export.h"

#include <vtkSmartPointer.h>

#include <QString>

#include <vector>

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;

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
