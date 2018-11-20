/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#include "iASimpleSlicerWidget.h"
#include "iAModalityTransfer.h"
#include "iASlicerData.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyle.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

iASimpleSlicerWidget::iASimpleSlicerWidget(QWidget * parent /*= 0*/, bool enableInteraction /*= false*/, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f), m_enableInteraction(enableInteraction)
{
	m_slicer = new iASlicer(this, iASlicerMode::XY, this,
		// TODO: do this in a better way?
		/*Qt::WindowFlags f = */f,
		/*bool decorations = */false); // Hide everything except the slice itself
}

iASimpleSlicerWidget::~iASimpleSlicerWidget()
{
	//delete m_slicer; // TODO uncomment?
}

void iASimpleSlicerWidget::setSlicerMode(iASlicerMode slicerMode)
{
	m_slicer->ChangeMode(slicerMode);
}

void iASimpleSlicerWidget::setSliceNumber(int sliceNumber)
{
	m_slicer->setSliceNumber(sliceNumber);
}

bool iASimpleSlicerWidget::hasHeightForWidth()
{
	return true;
}

int iASimpleSlicerWidget::heightForWidth(int width)
{
	return width;
}

void iASimpleSlicerWidget::update()
{
	m_slicer->update();
}

void iASimpleSlicerWidget::changeModality(QSharedPointer<iAModality> modality)
{
	vtkImageData *imageData = modality->GetImage().GetPointer();

	vtkColorTransferFunction* colorFunction = modality->GetTransfer()->getColorFunction();
	m_slicerTransform = vtkTransform::New();
	m_slicer->initializeData(imageData, m_slicerTransform, colorFunction);
	m_slicer->initializeWidget(imageData);
	m_slicer->disableInteractor();

	if (!m_enableInteraction) {
		vtkInteractorStyle *dummyStyle = vtkInteractorStyle::New();
		m_slicer->GetSlicerData()->GetInteractor()->SetInteractorStyle(dummyStyle);
	}

	double* origin = imageData->GetOrigin();
	int* extent = imageData->GetExtent();
	double* spacing = imageData->GetSpacing();

	double xc = origin[0] + 0.5*(extent[0] + extent[1])*spacing[0];
	double yc = origin[1] + 0.5*(extent[2] + extent[3])*spacing[1];
	double xd = (extent[1] - extent[0] + 1)*spacing[0];
	double yd = (extent[3] - extent[2] + 1)*spacing[1];

	vtkCamera *camera = m_slicer->GetCamera();
	double d = camera->GetDistance();

	camera->SetParallelScale(0.5*yd);
	camera->SetFocalPoint(xc, yc, 0.0);
	camera->SetPosition(xc, yc, +d);
}