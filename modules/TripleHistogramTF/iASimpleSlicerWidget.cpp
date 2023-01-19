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
#include "iASimpleSlicerWidget.h"

#include <iAChannelData.h>
#include <iATransferFunction.h>
#include <iASlicerImpl.h>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyle.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <QHBoxLayout>

iASimpleSlicerWidget::iASimpleSlicerWidget(QWidget * parent /*= 0*/, bool enableInteraction /*= false*/, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f), m_enableInteraction(enableInteraction),
	m_slicerTransform(vtkTransform::New())
{
	m_slicer = new iASlicerImpl(this, iASlicerMode::XY, /* magicLens = */ false, /*bool decorations = */false, m_slicerTransform); // Hide everything except the slice itself
	setLayout(new QHBoxLayout);
	layout()->setSpacing(0);
	layout()->addWidget(m_slicer);
}

iASimpleSlicerWidget::~iASimpleSlicerWidget()
{
	delete m_slicer;
}

void iASimpleSlicerWidget::applySettings(iASingleSlicerSettings const & settings)
{
	m_slicer->setup(settings);
}

void iASimpleSlicerWidget::setSlicerMode(iASlicerMode slicerMode)
{
	m_slicer->setMode(slicerMode);
}

iASlicerMode iASimpleSlicerWidget::getSlicerMode()
{
	return m_slicer->mode();
}

void iASimpleSlicerWidget::setSliceNumber(int sliceNumber)
{
	m_slicer->setSliceNumber(sliceNumber);
}

void iASimpleSlicerWidget::setCamera(vtkCamera* camera)
{
	m_slicer->setCamera(camera, false);
}

int iASimpleSlicerWidget::getSliceNumber()
{
	return m_slicer->sliceNumber();
}

bool iASimpleSlicerWidget::hasHeightForWidth() const
{
	return true;
}

int iASimpleSlicerWidget::heightForWidth(int width) const
{
	return width;
}

void iASimpleSlicerWidget::update()
{
	m_slicer->update();
}

void iASimpleSlicerWidget::changeData(vtkImageData* imageData, iATransferFunction* tf, QString const & name)
{
	vtkColorTransferFunction* colorFunction = tf->colorTF();
	m_slicer->addChannel(0, iAChannelData(name, imageData, colorFunction), true);
	m_slicer->disableInteractor();

	if (!m_enableInteraction)
	{
		vtkInteractorStyle *dummyStyle = vtkInteractorStyle::New();
		m_slicer->interactor()->SetInteractorStyle(dummyStyle);
	}

	double* origin = imageData->GetOrigin();
	int* extent = imageData->GetExtent();
	double* spacing = imageData->GetSpacing();

	double xc = origin[0] + 0.5*(extent[0] + extent[1])*spacing[0];
	double yc = origin[1] + 0.5*(extent[2] + extent[3])*spacing[1];
	double yd = (extent[3] - extent[2] + 1)*spacing[1];

	vtkCamera *camera = m_slicer->camera();
	double d = camera->GetDistance();

	camera->SetParallelScale(0.5*yd);
	camera->SetFocalPoint(xc, yc, 0.0);
	camera->SetPosition(xc, yc, +d);
}