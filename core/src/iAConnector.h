/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "open_iA_Core_export.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>
#include <vtkSmartPointer.h>

class vtkImageData;
class vtkImageExport;
class vtkImageImport;

/**
* \brief This class is needed for converting VTK images to ITK and vice versa. It is written to replace the ImageConverter of VTKEdge.
*/
class open_iA_Core_API iAConnector
{
public:
	// Type definitions
	typedef itk::ImageBase< 3 >					ImageBaseType;
	typedef ImageBaseType::Pointer				ImagePointer;
	typedef itk::ImageIOBase::IOComponentType	ITKScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType		ITKPixelType;
	typedef itk::ProcessObject::Pointer			ProcessObjectPointer;

	iAConnector();

	//! Set the VTK image and make it available as ITK image
	void setImage(vtkSmartPointer<vtkImageData> image);
	//! Sets the ITK image and make it available as VTK image
	void setImage(ImageBaseType * image);

	//! Get the VTK image
	vtkSmartPointer<vtkImageData> vtkImage() const;
	//! Get the ITK image
	ImageBaseType* itkImage() const;

	//! Get the data type of a single scalar (double, float, int, ...)
	ITKScalarPixelType itkScalarPixelType() const;
	//! Get the type of the pixel (SCALAR or RGBA)
	ITKPixelType itkPixelType();
	//! Set the linked ITK/VTK images as modified
	void modified();

protected:
	//! @{ Update one image using the respective other
	void updateImageITK();
	void updateImageVTK();
	//! @}
	void updateScalarType() const;
	void updatePixelType();

protected:
	ImagePointer m_ITKImage; //The pointer for the ITK image
	vtkSmartPointer<vtkImageData> m_VTKImage; //The pointer for the VTK image
	mutable ITKScalarPixelType m_itkScalarType;
	mutable bool m_isTypeInitialized;
	ITKPixelType m_itkPixelType;
	bool m_isPixelTypeInitialized;

	ProcessObjectPointer m_itkImporter;//itk::VTKImageImport
	ProcessObjectPointer m_itkExporter;//itk::VTKImageExport

	vtkSmartPointer<vtkImageExport> m_vtkExporter;
	vtkSmartPointer<vtkImageImport> m_vtkImporter;
};
