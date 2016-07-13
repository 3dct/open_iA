/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"

#include <itkCastImageFilter.h>
#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageIOBase.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

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
	typedef itk::ProcessObject::Pointer			ProcessObjectPointer;

	// Methods
	// -Constructor and Destructor
	iAConnector();
	~iAConnector();

	/** This function is needed for setting the VTK image and converting it */
	void SetImage(vtkSmartPointer<vtkImageData> image);
	/** This function is needed for setting the ITK image and converting it */
	void SetImage(ImageBaseType * image);

	// -Getters
	vtkSmartPointer<vtkImageData> GetVTKImage() const;
	ImageBaseType* GetITKImage() const;

	// -Helper methods
	ITKScalarPixelType GetITKScalarPixelType();
	void Modified();

protected:
	// -Update one modality using another
	void UpdateImageITK();
	void UpdateImageVTK();
	void UpdateScalarType();	

protected:
	ImagePointer m_ITKImage; //The pointer for the ITK image
	vtkSmartPointer<vtkImageData> m_VTKImage; //The pointer for the VTK image
	ITKScalarPixelType m_itkScalarType;
	bool m_isTypeInitialized;

	ProcessObjectPointer m_itkImporter;//itk::VTKImageImport
	ProcessObjectPointer m_itkExporter;//itk::VTKImageExport

	vtkSmartPointer<vtkImageExport> m_vtkExporter;
	vtkSmartPointer<vtkImageImport> m_vtkImporter;
};
