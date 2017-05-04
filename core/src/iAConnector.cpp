/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 

#include "pch.h"
#include "iAConnector.h"
#include "iAExtendedTypedCallHelper.h"
#include "iAToolsITK.h"

#include <vtkImageData.h>
#include <vtkImageExport.h>
#include <vtkImageImport.h>

#include <itkVTKImageImport.h>
#include <itkVTKImageExport.h>



/**
* This function will connect the given itk::VTKImageExport filter to
* the given vtkImageImport filter.
*/
template <typename ITK_Exporter, typename VTK_Importer>
void ConnectPipelines(ITK_Exporter exporter, VTK_Importer* importer)
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

/**
* This function will connect the given vtkImageExport filter to
* the given itk::VTKImageImport filter.
*/
template <typename VTK_Exporter, typename ITK_Importer>
void ConnectPipelines(VTK_Exporter* exporter, ITK_Importer importer)
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

/**
*  Will take care of instantiating the appropriate
*  ITK Export class corresponding to the actual pixel type of the
*  input image. */
template <class T >
void SetupPipelineITKtoVTK(
	iAConnector::ImagePointer	& imageBase,
	iAConnector::ProcessObjectPointer	& exporter,
	vtkImageImport					* importer)
{
	typedef itk::Image< T, 3 >					ImageType;
	typedef itk::VTKImageExport< ImageType >	ExportFilterType;
	typedef typename ExportFilterType::Pointer			ExportFilterPointer;


	ImageType * image = dynamic_cast<ImageType *>(imageBase.GetPointer());
	if (!image)
		return;

	ExportFilterPointer _exporter = ExportFilterType::New();
	_exporter->SetInput(image);
	exporter = _exporter;

	ConnectPipelines(_exporter, importer);
	_exporter->Update();
	importer->Update();
}

iAConnector::iAConnector() :
	m_ITKImage(ImageBaseType::New()),
	m_VTKImage(vtkSmartPointer<vtkImageData>::New()),
	m_itkScalarType(itk::ImageIOBase::UNKNOWNCOMPONENTTYPE),
	m_isTypeInitialized(false),
	m_itkPixelType( itk::ImageIOBase::UNKNOWNPIXELTYPE ),
	m_isPixelTypeInitialized( false ),
	m_vtkImporter(vtkSmartPointer<vtkImageImport>::New()),
	m_vtkExporter(vtkSmartPointer<vtkImageExport>::New())
{}

iAConnector::~iAConnector()
{}


void iAConnector::SetImage( ImageBaseType * image )
{
	m_isTypeInitialized = false;
	if( this->m_ITKImage.GetPointer() == image )
		return;
	m_ITKImage = image;
	UpdateImageVTK();
}

void iAConnector::UpdateImageVTK()
{
	iAConnector::ITKScalarPixelType componentType = GetITKScalarPixelType();
	iAConnector::ITKPixelType pixelType = GetITKPixelType();
	m_VTKImage = 0;
	ITK_EXTENDED_TYPED_CALL(SetupPipelineITKtoVTK, componentType, pixelType, m_ITKImage, m_itkExporter, m_vtkImporter);
	m_VTKImage = m_vtkImporter->GetOutput();
}

void iAConnector::SetImage(vtkSmartPointer<vtkImageData> imageData)
{
	if (m_VTKImage == imageData)
		return;
	m_VTKImage = imageData;
	UpdateImageITK();
}

void iAConnector::UpdateImageITK()
{
	m_isTypeInitialized = false;
	int scalarType = m_VTKImage->GetScalarType();
	int compCount = m_VTKImage->GetNumberOfScalarComponents();
	m_ITKImage = 0;
	VTK_EXTENDED_TYPED_CALL( SetupPipelineVTKtoITK, scalarType, compCount, m_VTKImage, m_vtkExporter, m_itkImporter );
	m_ITKImage = dynamic_cast<ImageBaseType*>(m_itkImporter->GetOutputs()[0].GetPointer());
}

vtkSmartPointer<vtkImageData> iAConnector::GetVTKImage() const
{
	return m_VTKImage;
}

iAConnector::ImageBaseType* iAConnector::GetITKImage() const
{
	return m_ITKImage;
}

void iAConnector::UpdateScalarType()
{
	m_isTypeInitialized = true;
	m_itkScalarType = ::GetITKScalarPixelType(m_ITKImage);
}

iAConnector::ITKScalarPixelType iAConnector::GetITKScalarPixelType()
{
	if (!m_isTypeInitialized)
		UpdateScalarType();
	return m_itkScalarType;
}

void iAConnector::UpdatePixelType()
{
	m_isPixelTypeInitialized = true;
	m_itkPixelType = ::GetITKPixelType( m_ITKImage );
}

iAConnector::ITKPixelType iAConnector::GetITKPixelType()
{
	if ( !m_isPixelTypeInitialized )
		UpdatePixelType();
	return m_itkPixelType;
}

void iAConnector::Modified()
{
	m_ITKImage->Modified();
	m_VTKImage->Modified();
	UpdateScalarType();
}
