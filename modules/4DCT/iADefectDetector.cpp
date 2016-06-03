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
 
#include "pch.h"
#include "iADefectDetector.h"
// iA
#include "iAEndpointsExtractor.h"
// vtk
#include <QVTKWidget.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
// itk
#include <itkSpatialObjectToImageFilter.h>
#include <itkEllipseSpatialObject.h>
#include <itkImageFileWriter.h>
#include <itkImage.h>

void GenerateTempleateImage(double size, itk::Image<int, 3>::Pointer imageOut)
{
	const int Dimension = 3;
	typedef itk::Image<int, Dimension> ImageType;

	// create a spatial object
	typedef itk::EllipseSpatialObject<Dimension> EllipseType;
	EllipseType::Pointer ellipse = EllipseType::New();
	ellipse->SetRadius(size);

	typedef EllipseType::TransformType TransformType;
	TransformType::Pointer transform = TransformType::New();
	transform->SetIdentity();

	TransformType::OutputVectorType translation;
	translation[0] = size;
	translation[1] = size;
	translation[2] = size;
	transform->Translate(translation);

	ellipse->SetObjectToParentTransform(transform);

	// create a filter
	typedef itk::SpatialObjectToImageFilter<EllipseType, ImageType> SpatialObjectToImageFilterType;
	SpatialObjectToImageFilterType::Pointer imageFilter = SpatialObjectToImageFilterType::New();

	ImageType::SizeType imgSize;
	imgSize[0] = size * 2;
	imgSize[1] = size * 2;
	imgSize[2] = size * 2;
	imageFilter->SetSize(imgSize);

	ImageType::SpacingType spacing;
	spacing[0] = 1;
	spacing[1] = 1;
	spacing[2] = 1;
	imageFilter->SetSpacing(spacing);

	imageFilter->SetInput(ellipse);
	ellipse->SetDefaultInsideValue(1);
	ellipse->SetDefaultOutsideValue(0);
	imageFilter->SetUseObjectValue(true);
	imageFilter->SetOutsideValue(0);

	// setup writer
	typedef itk::ImageFileWriter<ImageType> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName("D:\\work\\Datasets\\4DXCT_Downsampled\\output\\template.mhd");
	writer->SetInput(imageFilter->GetOutput());

	try
	{
		imageFilter->Update();
		writer->Update();
	}
	catch(itk::ExceptionObject&/* e*/)
	{
		
	}
}

void iADefectDetector::findFiberPullOuts(vtkImageData* image, Endpoint endpoint, vtkImageData* poresSeg, vtkImageData* fibersSeg)
{
	vtkSmartPointer<vtkImageData> movingImg = vtkSmartPointer<vtkImageData>::New();
	int pos[3];
	endpoint.GetPosition(pos);
	fillMovingImage(fibersSeg, movingImg, pos, 10);

	typedef itk::Image<int, 3> ImageType;
	ImageType::Pointer img;

	GenerateTempleateImage(5, img);
}

void iADefectDetector::fillMovingImage(vtkImageData * refImg, vtkImageData * output, int point[3], double radius)
{
	int dimLength = 2 * radius + 1;
	int dim[3] = { dimLength, dimLength, dimLength };
	output->SetDimensions(dim);
	output->AllocateScalars(VTK_CHAR, 1);
	void * buffer = output->GetScalarPointer();
	memset(buffer, 0, sizeof(char) * dim[0] * dim[1] * dim[2]);
}
