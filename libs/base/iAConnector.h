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

#include "iAbase_export.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>

#include <vtkImageExport.h>
#include <vtkImageImport.h>
#include <vtkSmartPointer.h>

class vtkImageData;

//! Converts VTK images to ITK and vice versa. It is written to replace the ImageConverter of VTKEdge.
class iAbase_API iAConnector
{
public:
	// Type definitions - unify with iAITKIO definitions, and with defines.h DIM!
	typedef itk::ImageBase< 3 >					ImageBaseType;
	typedef ImageBaseType::Pointer				ImagePointer;
	typedef itk::ImageIOBase::IOComponentType	ITKScalarPixelType;
	typedef itk::ImageIOBase::IOPixelType		ITKPixelType;
	typedef itk::ProcessObject::Pointer			ProcessObjectPointer;

	iAConnector();

	//! Set the VTK image and make it available as ITK image
	void setImage(vtkSmartPointer<vtkImageData> image);
	//! Sets the ITK image and make it available as VTK image
	void setImage(ImageBaseType* image);
	//! Set the linked ITK/VTK images as modified
	void modified();

	//! Get the VTK image
	vtkSmartPointer<vtkImageData> vtkImage() const;
	//! Get the ITK image
	ImageBaseType* itkImage() const;

	//! Get the data type of a single scalar (double, float, int, ...)
	ITKScalarPixelType itkScalarPixelType() const;
	//! Get the type of the pixel (SCALAR or RGBA)
	ITKPixelType itkPixelType() const;

private:
	//! @{ Update one image using the respective other
	void updateImageITK() const;
	void updateImageVTK() const;
	//! @}
	void updateScalarType() const;
	void updatePixelType() const;

	mutable ImagePointer m_ITKImage;                   //!< The pointer for the ITK image
	mutable vtkSmartPointer<vtkImageData> m_VTKImage;  //!< The pointer for the VTK image
	mutable ITKScalarPixelType m_itkScalarType;//!< cached ITK scalar type
	mutable bool m_isTypeInitialized;          //!< indication whether cached scalar type (m_itkScalarType) is already initialized
	mutable ITKPixelType m_itkPixelType;       //!< ITK pixel type (possible values: SCALAR or RGBA)
	mutable bool m_isPixelTypeInitialized;     //!< indication whether cached pixel type (m_itkPixelType) is already initialized

	//! @{ ITK/VTK export/import filters:
	mutable ProcessObjectPointer m_itkImporter;
	mutable ProcessObjectPointer m_itkExporter;
	vtkSmartPointer<vtkImageExport> m_vtkExporter;
	vtkSmartPointer<vtkImageImport> m_vtkImporter;
	//! @}
};
