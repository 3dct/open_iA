// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAConnector.h"

#include "iAExtendedTypedCallHelper.h"
#include "iAToolsITK.h"

#include <itkVTKImageImport.h>
#include <itkVTKImageExport.h>

#include <vtkImageData.h>

//! Connects the given vtkImageExport / itk::VTKImageExport filter to the given vtkImageImport / itk::VTKImageImport filter.
template <typename ExporterT, typename ImporterT>
void ConnectPipelines(ExporterT exporter, ImporterT importer)
{
	importer->SetUpdateInformationCallback(exporter->GetUpdateInformationCallback());
	importer->SetPipelineModifiedCallback(exporter->GetPipelineModifiedCallback());
	importer->SetWholeExtentCallback(exporter->GetWholeExtentCallback());
	importer->SetSpacingCallback(exporter->GetSpacingCallback());
	importer->SetOriginCallback(exporter->GetOriginCallback());
	importer->SetScalarTypeCallback(exporter->GetScalarTypeCallback());
	importer->SetNumberOfComponentsCallback(exporter->GetNumberOfComponentsCallback());
	importer->SetPropagateUpdateExtentCallback(exporter->GetPropagateUpdateExtentCallback());
	importer->SetUpdateDataCallback(exporter->GetUpdateDataCallback());
	importer->SetDataExtentCallback(exporter->GetDataExtentCallback());
	importer->SetBufferPointerCallback(exporter->GetBufferPointerCallback());
	importer->SetCallbackUserData(exporter->GetCallbackUserData());
}

template <class  T>
void SetupPipelineVTKtoITK(
	vtkImageData* inputImage,
	vtkImageExport * vtkExporter,
	iAConnector::ProcessObjectPointer & itkImporter)
{
	typedef itk::Image< T, 3 >						OutputImageType;
	typedef itk::VTKImageImport< OutputImageType >	ImageImportType;
	typename ImageImportType::Pointer _itkImporter = ImageImportType::New();
	itkImporter = _itkImporter;
	vtkExporter->SetInputData(inputImage);
	ConnectPipelines(vtkExporter, _itkImporter);
	itkImporter->Update();
}

//! Takes care of instantiating the appropriate ITK Export class corresponding
//! to the actual pixel type of the input image.
template <class T >
void SetupPipelineITKtoVTK(
	iAITKIO::ImagePointer	& imageBase,
	iAConnector::ProcessObjectPointer	& exporter,
	vtkImageImport					* importer)
{
	typedef itk::Image< T, 3 >					ImageType;
	typedef itk::VTKImageExport< ImageType >	ExportFilterType;
	typedef typename ExportFilterType::Pointer			ExportFilterPointer;


	ImageType * image = dynamic_cast<ImageType *>(imageBase.GetPointer());
	if (!image)
	{
		return;
	}

	ExportFilterPointer _exporter = ExportFilterType::New();
	_exporter->SetInput(image);
	exporter = _exporter;

	ConnectPipelines(_exporter, importer);
	_exporter->Update();
	importer->Update();
}


iAConnector::iAConnector() :
	m_ITKImage(nullptr),
	m_VTKImage(nullptr),
	m_itkScalarType(iAITKIO::ScalarType::UNKNOWNCOMPONENTTYPE),
	m_isTypeInitialized(false),
	m_itkPixelType(iAITKIO::PixelType::UNKNOWNPIXELTYPE ),
	m_isPixelTypeInitialized( false ),
	m_vtkExporter(vtkSmartPointer<vtkImageExport>::New()),
	m_vtkImporter(vtkSmartPointer<vtkImageImport>::New())
{}

void iAConnector::setImage(iAITKIO::ImagePtr image)
{
	if (this->m_ITKImage.GetPointer() == image)
	{
		return;
	}
	m_isTypeInitialized = false;
	m_isPixelTypeInitialized = false;
	m_ITKImage = image;
	m_VTKImage = nullptr;
}

void iAConnector::updateImageVTK() const
{
	auto scalarType = itkScalarType();
	auto pixelType = itkPixelType();
	ITK_EXTENDED_TYPED_CALL(SetupPipelineITKtoVTK, scalarType, pixelType, m_ITKImage, m_itkExporter, m_vtkImporter);
	m_VTKImage = m_vtkImporter->GetOutput();
}

void iAConnector::setImage(vtkSmartPointer<vtkImageData> imageData)
{
	if (m_VTKImage == imageData)
	{
		return;
	}
	m_isTypeInitialized = false;
	m_isPixelTypeInitialized = false;
	m_VTKImage = imageData;
	m_ITKImage = nullptr;
}

void iAConnector::updateImageITK() const
{
	m_isTypeInitialized = false;
	int scalarType = m_VTKImage->GetScalarType();
	int compCount = m_VTKImage->GetNumberOfScalarComponents();
	VTK_EXTENDED_TYPED_CALL(SetupPipelineVTKtoITK, scalarType, compCount, m_VTKImage, m_vtkExporter, m_itkImporter);
	m_ITKImage = dynamic_cast<iAITKIO::ImagePtr>(m_itkImporter->GetOutputs()[0].GetPointer());
}

vtkSmartPointer<vtkImageData> iAConnector::vtkImage() const
{
	if (!m_VTKImage && m_ITKImage)
	{
		updateImageVTK();
	}
	// if (!m_VTKImage ...
	return m_VTKImage;
}

iAITKIO::ImagePtr iAConnector::itkImage() const
{
	if (!m_ITKImage && m_VTKImage)
	{
		updateImageITK();
	}
	// if (!m_ITKImage ...
	return m_ITKImage;
}

void iAConnector::updateScalarType() const
{
	if (!m_ITKImage)
	{
		updateImageITK();
	}
	m_isTypeInitialized = true;
	m_itkScalarType = ::itkScalarType(m_ITKImage);
}

iAITKIO::ScalarType iAConnector::itkScalarType() const
{
	if (!m_isTypeInitialized)
	{
		updateScalarType();
	}
	return m_itkScalarType;
}

void iAConnector::updatePixelType() const
{
	if (!m_ITKImage)
	{
		updateImageITK();
	}
	m_isPixelTypeInitialized = true;
	m_itkPixelType = ::itkPixelType( m_ITKImage );
}

iAITKIO::PixelType iAConnector::itkPixelType() const
{
	if (!m_isPixelTypeInitialized)
	{
		updatePixelType();
	}
	return m_itkPixelType;
}

void iAConnector::modified()
{
	assert(m_ITKImage || m_VTKImage);
	if (m_ITKImage)
	{
		m_ITKImage->Modified();
	}
	if (m_VTKImage)
	{
		m_VTKImage->Modified();
	}
	m_isTypeInitialized = false;
	m_isPixelTypeInitialized = false;
}
