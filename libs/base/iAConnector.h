// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include "iAITKIO.h"

#include <vtkImageExport.h>
#include <vtkImageImport.h>
#include <vtkSmartPointer.h>

class vtkImageData;

//! Converts VTK images to ITK and vice versa. It is written to replace the ImageConverter of VTKEdge.
class iAbase_API iAConnector
{
public:
	typedef itk::ProcessObject::Pointer			ProcessObjectPointer;

	iAConnector();

	//! Set the VTK image and make it available as ITK image
	void setImage(vtkSmartPointer<vtkImageData> image);
	//! Sets the ITK image and make it available as VTK image
	void setImage(iAITKIO::ImagePtr image);
	//! Set the linked ITK/VTK images as modified
	void modified();

	//! Get the VTK image
	vtkSmartPointer<vtkImageData> vtkImage() const;
	//! Get the ITK image
	iAITKIO::ImagePtr itkImage() const;

	//! Get the data type of a single scalar (double, float, int, ...)
	iAITKIO::ScalarType itkScalarType() const;
	//! Get the type of the pixel (SCALAR or RGBA)
	iAITKIO::PixelType itkPixelType() const;

private:
	//! @{ Update one image using the respective other
	void updateImageITK() const;
	void updateImageVTK() const;
	//! @}
	void updateScalarType() const;
	void updatePixelType() const;

	mutable iAITKIO::ImagePointer m_ITKImage;          //!< The pointer for the ITK image
	mutable vtkSmartPointer<vtkImageData> m_VTKImage;  //!< The pointer for the VTK image
	mutable iAITKIO::ScalarType m_itkScalarType;//!< cached ITK scalar type
	mutable bool m_isTypeInitialized;          //!< indication whether cached scalar type (m_itkScalarType) is already initialized
	mutable iAITKIO::PixelType m_itkPixelType; //!< ITK pixel type (possible values: SCALAR or RGBA)
	mutable bool m_isPixelTypeInitialized;     //!< indication whether cached pixel type (m_itkPixelType) is already initialized

	//! @{ ITK/VTK export/import filters:
	mutable ProcessObjectPointer m_itkImporter;
	mutable ProcessObjectPointer m_itkExporter;
	vtkSmartPointer<vtkImageExport> m_vtkExporter;
	vtkSmartPointer<vtkImageImport> m_vtkImporter;
	//! @}
};
