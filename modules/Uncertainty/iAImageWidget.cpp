// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

#include <iASlicerImpl.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>

#include <QApplication>
#include <QHBoxLayout>

iAImageWidget::iAImageWidget(vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkScalarsToColors> lut):
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
	m_slicer = new iASlicerImpl(this, iASlicerMode::XY, false, true);
	setLayout(new QHBoxLayout);
	layout()->setSpacing(0);
	layout()->addWidget(m_slicer);
	m_slicer->addChannel(0, iAChannelData("", img, m_lut), true);
	StyleChanged();
}


void iAImageWidget::StyleChanged()
{
	QColor bgColor = QApplication::palette().color(QWidget::backgroundRole());

	//m_renderer->SetBackground(bgColor.red() / 255.0, bgColor.green() / 255.0, bgColor.blue() / 255.0);
	m_slicer->setBackground(bgColor);
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
	int const * ext = m_slicer->channel(0)->input()->GetExtent();
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
