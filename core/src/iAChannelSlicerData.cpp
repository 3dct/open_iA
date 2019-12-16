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

#include "iAChannelData.h"
#include "iAConsole.h"
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
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include <QThread>

iAChannelSlicerData::iAChannelSlicerData(iAChannelData const & chData, int mode):
	m_reslicer(vtkSmartPointer<vtkImageReslice>::New()),
	m_colormapper(vtkSmartPointer<vtkImageMapToColors>::New()),
	m_imageActor(vtkSmartPointer<vtkImageActor>::New()),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_name(chData.name()),
	m_cTF(nullptr),
	m_oTF(nullptr),
	m_contourFilter(vtkSmartPointer<vtkMarchingContourFilter>::New()),
	m_contourMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_contourActor(vtkSmartPointer<vtkActor>::New()),
	m_enabled(false)
{
	m_reslicer->SetOutputDimensionality(2);
	m_reslicer->SetInterpolationModeToCubic();
	m_reslicer->InterpolateOn();
	m_reslicer->AutoCropOutputOn();
	m_reslicer->SetNumberOfThreads(QThread::idealThreadCount());
	assign(chData.image());
	m_imageActor->GetMapper()->BorderOn();
	updateResliceAxesDirectionCosines(mode);
	setupOutput(chData.colorTF(), chData.opacityTF());
	initContours();
}

void iAChannelSlicerData::setResliceAxesOrigin(double x, double y, double z)
{
	m_reslicer->SetResliceAxesOrigin(x, y, z);
	if (m_enabled)
	{
		m_reslicer->Update();
		m_colormapper->Update();
	}
	m_imageActor->SetInputData(m_colormapper->GetOutput());
}

void iAChannelSlicerData::resliceAxesOrigin(double * origin)
{
	m_reslicer->GetResliceAxesOrigin(origin);
}

void iAChannelSlicerData::assign(vtkSmartPointer<vtkImageData> imageData)
{
	m_reslicer->SetInputData(imageData);
	m_reslicer->SetInformationInput(imageData);
}

void iAChannelSlicerData::setupOutput(vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	if (input()->GetNumberOfScalarComponents() == 1)
	{
		m_cTF = ctf;
		m_oTF = otf;
		updateLUT();
		m_colormapper->SetLookupTable(m_oTF ? m_lut : m_cTF);
		m_colormapper->PassAlphaToOutputOn();
	}
	else
	{
		m_colormapper->SetLookupTable(nullptr);
		if (input()->GetNumberOfScalarComponents() == 3)
		{
			m_colormapper->SetOutputFormatToRGB();
		}
		else if (input()->GetNumberOfScalarComponents() == 4)
		{
			m_colormapper->SetOutputFormatToRGBA();
		}
		else
		{
			DEBUG_LOG(QString("Unsupported number of components (%1)!").arg(input()->GetNumberOfScalarComponents()));
		}
	}
	m_colormapper->SetInputConnection(m_reslicer->GetOutputPort());
	m_colormapper->Update();
	m_imageActor->SetInputData(m_colormapper->GetOutput());
}

void iAChannelSlicerData::updateLUT()
{
	if (!m_oTF)
		return;
	double rgb[3];
	double range[2];
	input()->GetScalarRange(range);
	m_lut->SetRange(range);
	const int numCols = 1024;
	m_lut->SetNumberOfTableValues(numCols);
	double scalVal = range[0];
	double scalValDelta = (range[1] - range[0]) / (numCols - 1);
	for (int i = 0; i < numCols; ++i)
	{
		m_cTF->GetColor(scalVal, rgb);
		double alpha = m_oTF->GetValue(scalVal);
		m_lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], alpha);
		scalVal += scalValDelta;
	}
	m_lut->Build();
}

void iAChannelSlicerData::update(iAChannelData const & chData)
{
	assign(chData.image());
	m_name = chData.name();
	m_reslicer->Update();

	setupOutput(chData.colorTF(), chData.opacityTF());
}

void iAChannelSlicerData::updateResliceAxesDirectionCosines(int mode)
{
	switch (mode)
	{
	case iASlicerMode::YZ:
		m_reslicer->SetResliceAxesDirectionCosines(0,1,0,  0,0,1,  1,0,0);  //yz
		break;
	case iASlicerMode::XY:
		m_reslicer->SetResliceAxesDirectionCosines(1,0,0,  0,1,0,  0,0,1);  //xy
		break;
	case iASlicerMode::XZ:
		m_reslicer->SetResliceAxesDirectionCosines(1,0,0,  0,0,1,  0,-1,0); //xz
		break;
	default:
		break;
	}
}

/*
bool iAChannelSlicerData::isInitialized() const
{
	return m_isInitialized;
}
*/

vtkScalarsToColors* iAChannelSlicerData::colorTF()
{
	return m_cTF;
}

vtkPiecewiseFunction * iAChannelSlicerData::opacityTF()
{
	return m_oTF;
}

void iAChannelSlicerData::updateMapper()
{
	m_colormapper->Update();
}

void iAChannelSlicerData::setTransform(vtkAbstractTransform * transform)
{
	m_reslicer->SetResliceTransform(transform);
	//if (input())
	//	m_reslicer->UpdateWholeExtent(); // TODO: check if we need this here
}

void iAChannelSlicerData::updateReslicer()
{
	m_reslicer->Update();
}

bool iAChannelSlicerData::isEnabled() const
{
	return m_enabled;
}

QString const & iAChannelSlicerData::name() const
{
	return m_name;
}

vtkImageActor * iAChannelSlicerData::imageActor()
{
	return m_imageActor;
}

vtkImageData * iAChannelSlicerData::input() const
{
	return dynamic_cast<vtkImageData*>(m_reslicer->GetInput());
}

vtkImageData * iAChannelSlicerData::output() const
{
	return m_reslicer->GetOutput();
}

vtkImageReslice * iAChannelSlicerData::reslicer() const
{
	return m_reslicer;
}

double const * iAChannelSlicerData::actorPosition() const
{
	return m_imageActor->GetPosition();
}

void iAChannelSlicerData::setActorPosition(double x, double y, double z)
{
	m_imageActor->SetPosition(x, y, z);
}

void iAChannelSlicerData::setActorOpacity(double opacity)
{
	m_imageActor->SetOpacity(opacity);
}

void iAChannelSlicerData::setMovable(bool movable)
{
	m_imageActor->SetPickable(movable);
	m_imageActor->SetDragable(movable);
}

void iAChannelSlicerData::setInterpolate(bool interpolate)
{
	m_imageActor->SetInterpolate(interpolate);
}

void iAChannelSlicerData::setEnabled(vtkRenderer* ren, bool enable)
{
	m_enabled = enable;
	if (enable)
		ren->AddActor(m_imageActor);
	else
		ren->RemoveActor(m_imageActor);
}

void iAChannelSlicerData::setSlabNumberOfSlices(int slices)
{
	m_reslicer->SetSlabNumberOfSlices(slices);
}

void iAChannelSlicerData::setSlabMode(int mode)
{
	m_reslicer->SetSlabMode(mode);
}

void iAChannelSlicerData::initContours()
{
	m_contourFilter->SetInputConnection(m_reslicer->GetOutputPort());
	m_contourFilter->UseScalarTreeOn();
	m_contourFilter->SetComputeGradients(false);
	m_contourFilter->SetComputeNormals(false);
	m_contourMapper->SetInputConnection(m_contourFilter->GetOutputPort());
	m_contourMapper->SetResolveCoincidentTopology(VTK_RESOLVE_POLYGON_OFFSET);
	m_contourMapper->ScalarVisibilityOff();
	m_contourActor->SetMapper(m_contourMapper);
}

void iAChannelSlicerData::setContours(int numberOfContours, double contourMin, double contourMax)
{
	m_contourFilter->GenerateValues(numberOfContours, contourMin, contourMax);
}

void iAChannelSlicerData::setContours(int numberOfContours, double const * contourValues)
{
	m_contourFilter->SetNumberOfContours(numberOfContours);
	for (int i = 0; i < numberOfContours; ++i)
		m_contourFilter->SetValue(i, contourValues[i]);
	m_contourFilter->Update();
}

void iAChannelSlicerData::setShowContours(vtkRenderer* ren, bool enable)
{
	if (enable)
		ren->AddActor(m_contourActor);
	else
		ren->RemoveActor(m_contourActor);
}

void iAChannelSlicerData::setContourLineParams(double lineWidth, bool dashed)
{
	m_contourActor->GetProperty()->SetLineWidth(lineWidth);
	if (dashed)
		m_contourActor->GetProperty()->SetLineStipplePattern(0xff00);
}

void iAChannelSlicerData::setContoursColor(double * rgb)
{
	m_contourActor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
}

void iAChannelSlicerData::setContoursOpacity(double opacity)
{
	m_contourActor->GetProperty()->SetOpacity(opacity);
}
