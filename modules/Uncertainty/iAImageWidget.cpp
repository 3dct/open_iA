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
#include "iAImageWidget.h"

// separate solution based on vtkImageSlice:
/*
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkRenderer.h>
*/

// iASlicer-based solution:
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iASlicerSettings.h>
#include <iASlicer.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkTransform.h>

#include <QHBoxLayout>

iAImageWidget::iAImageWidget(vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkScalarsToColors> lut):
	m_transform(vtkSmartPointer<vtkTransform>::New()),
	m_lut(lut)
{
	/*
	auto ctf = GetDefaultColorTransferFunction(img);
	auto mapper = vtkSmartPointer<vtkImageResliceMapper>::New();
	mapper->SetInputData(img);
	// additional mapper settings:
	// vtkNew<vtkPlane> plane;
	// plane->SetOrigin(0, 0, 0);
	// plane->SetNormal(0, 0, 1);
	// resliceMapper->SetSlicePlane(plane);
	// resliceMapper->SetSlabThickness(0.1);
	mapper->SliceAtFocalPointOn();
	mapper->BorderOn();

	auto slice = vtkSmartPointer<vtkImageSlice>::New();
	slice->SetMapper(mapper);

	auto imgProp = vtkSmartPointer<vtkImageProperty>::New();
	imgProp->SetLookupTable(ctf);
	slice->SetProperty(imgProp);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->AddViewProp(slice);
	m_renderer->ResetCamera();
	auto renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	renderWindow->AddRenderer(m_renderer);
	SetRenderWindow(renderWindow);
	*/
	m_slicer = new iASlicer(this, iASlicerMode::XY, false, true, m_transform);
	setLayout(new QHBoxLayout);
	layout()->setSpacing(0);
	layout()->addWidget(m_slicer);
	m_slicer->setup(iASingleSlicerSettings());
	m_slicer->addChannel(0, iAChannelData("", img, m_lut), true);
	StyleChanged();
}


void iAImageWidget::StyleChanged()
{
	QColor bgColor = QWidget::palette().color(QWidget::backgroundRole());
	
	//m_renderer->SetBackground(bgColor.red() / 255.0, bgColor.green() / 255.0, bgColor.blue() / 255.0);
	m_slicer->setBackground(bgColor.red() / 255.0, bgColor.green() / 255.0, bgColor.blue() / 255.0);
}


void iAImageWidget::SetMode(int slicerMode)
{
	m_slicer->setMode(static_cast<iASlicerMode>(slicerMode));
	m_slicer->update();
}

void iAImageWidget::SetSlice(int sliceNumber)
{
	m_slicer->setSliceNumber(sliceNumber);
	m_slicer->update();
}

int iAImageWidget::GetSliceCount() const
{
	int const * ext = m_slicer->getChannel(0)->input()->GetExtent();
	switch (m_slicer->mode())
	{
		case XZ: return ext[3] - ext[2] + 1;
		case YZ: return ext[1] - ext[0] + 1;
		default:
		case XY: return ext[5] - ext[4] + 1;
	}
}


iASlicer* iAImageWidget::GetSlicer()
{
	return m_slicer;
}
