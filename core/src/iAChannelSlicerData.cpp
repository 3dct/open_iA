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

#include "iAChannelSlicerData.h"
#include "iAChannelVisualizationData.h"
#include "iASlicerMode.h"

#include <vtkActor.h>
#include <vtkImageReslice.h>
#include <vtkImageMapToColors.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkMarchingContourFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTransform.h>

#include <QThread>

iAChannelSlicerData::iAChannelSlicerData() :
	image(NULL),
	reslicer(vtkImageReslice::New()),
	m_colormapper(vtkImageMapToColors::New()),
	imageActor(vtkImageActor::New()),
	m_isInitialized(false),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	cFilter(vtkSmartPointer<vtkMarchingContourFilter>::New()),
	cMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	cActor(vtkSmartPointer<vtkActor>::New()),
	m_ctf(nullptr),
	m_otf(nullptr)
{}

iAChannelSlicerData::~iAChannelSlicerData()
{
	m_colormapper->Delete();
	imageActor->Delete();
	reslicer->ReleaseDataFlagOn();
	reslicer->Delete();
}

void iAChannelSlicerData::setResliceAxesOrigin(double x, double y, double z)
{
	reslicer->SetResliceAxesOrigin(x, y, z);
	reslicer->Update();
	m_colormapper->Update();
	imageActor->SetInputData(m_colormapper->GetOutput());
	imageActor->GetMapper()->BorderOn();
}

void iAChannelSlicerData::assign(vtkSmartPointer<vtkImageData> imageData, QColor const &col)
{
	m_color = col;
	image = imageData;
	reslicer->SetInputData(imageData);
	reslicer->SetInformationInput(imageData);
}

void iAChannelSlicerData::setupOutput(vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	m_ctf = ctf;
	m_otf = otf;
	updateLUT();
	m_colormapper->SetLookupTable( m_otf ? m_lut : m_ctf);//colormapper->SetLookupTable( ctf );
	m_colormapper->PassAlphaToOutputOn();
	m_colormapper->SetInputConnection(reslicer->GetOutputPort());
	m_colormapper->Update();
	imageActor->SetInputData(m_colormapper->GetOutput());
}

void iAChannelSlicerData::updateLUT()
{
	if (!m_otf)
		return;
	double rgb[3];
	double range[2];image->GetScalarRange(range);
	m_lut->SetRange(range);
	const int numCols = 1024;
	m_lut->SetNumberOfTableValues(numCols);
	double scalVal = range[0];
	double scalValDelta = (range[1] - range[0]) / (numCols - 1);
	for (int i = 0; i < numCols; ++i)
	{
		m_ctf->GetColor(scalVal, rgb);
		double alpha = m_otf->GetValue(scalVal);
		m_lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], alpha);
		scalVal += scalValDelta;
	}
	m_lut->Build();
}

void iAChannelSlicerData::init(iAChannelVisualizationData * chData, int mode)
{
	m_isInitialized = true;
	assign(chData->getImage(), chData->getColor());
	m_name = chData->getName();

	reslicer->SetOutputDimensionality(2);
	reslicer->SetInterpolationModeToCubic();
	reslicer->InterpolateOn();
	reslicer->AutoCropOutputOn();
	reslicer->SetNumberOfThreads(QThread::idealThreadCount());
	updateResliceAxesDirectionCosines(mode);
	setupOutput(chData->getCTF(), chData->getOTF());
	initContours();
}

void iAChannelSlicerData::updateResliceAxesDirectionCosines(int mode)
{
	switch (mode)
	{
	case iASlicerMode::YZ:
		reslicer->SetResliceAxesDirectionCosines(0,1,0,  0,0,1,  1,0,0);  //yz
		break;
	case iASlicerMode::XY:
		reslicer->SetResliceAxesDirectionCosines(1,0,0,  0,1,0,  0,0,1);  //xy
		break;
	case iASlicerMode::XZ:
		reslicer->SetResliceAxesDirectionCosines(1,0,0,  0,0,1,  0,-1,0); //xz
		break;
	default:
		break;
	}
}

void iAChannelSlicerData::reInit(iAChannelVisualizationData * chData)
{
	assign(chData->getImage(), chData->getColor());
	m_name = chData->getName();

	reslicer->UpdateWholeExtent();
	reslicer->Update();

	setupOutput(chData->getCTF(), chData->getOTF());
}

bool iAChannelSlicerData::isInitialized()
{
	return m_isInitialized;
}

vtkScalarsToColors* iAChannelSlicerData::getLookupTable()
{
	return m_colormapper->GetLookupTable();
}

void iAChannelSlicerData::updateMapper()
{
	m_colormapper->Update();
}

QColor iAChannelSlicerData::getColor() const
{
	return m_color;
}

void iAChannelSlicerData::assignTransform(vtkTransform * transform)
{
	reslicer->SetResliceTransform(transform);
}

void iAChannelSlicerData::updateReslicer()
{
	reslicer->Update();
}

void iAChannelSlicerData::setContours(int num, const double * contourVals)
{
	cFilter->SetNumberOfContours(num);
	for (int i = 0; i < num; ++i)
		cFilter->SetValue(i, contourVals[i]);
	cFilter->Update();
}

void iAChannelSlicerData::setContoursColor(double * rgb)
{
	cActor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
}

void iAChannelSlicerData::setShowContours(bool show)
{
	cActor->SetVisibility(show);
}

void iAChannelSlicerData::setContourLineParams(double lineWidth, bool dashed)
{
	cActor->GetProperty()->SetLineWidth(lineWidth);
	if (dashed)
		cActor->GetProperty()->SetLineStipplePattern(0xff00);
}

void iAChannelSlicerData::setContoursOpacity(double opacity)
{
	cActor->GetProperty()->SetOpacity(opacity);
}

QString iAChannelSlicerData::getName() const
{
	return m_name;
}

void iAChannelSlicerData::initContours()
{
	cFilter->SetInputConnection(reslicer->GetOutputPort());
	cFilter->UseScalarTreeOn();
	cFilter->SetComputeGradients(false);
	cFilter->SetComputeNormals(false);
	cMapper->SetInputConnection(cFilter->GetOutputPort());
	cMapper->SetResolveCoincidentTopology(VTK_RESOLVE_POLYGON_OFFSET);
	cMapper->ScalarVisibilityOff();
	cActor->SetMapper(cMapper);
	cActor->SetVisibility(false);
}
