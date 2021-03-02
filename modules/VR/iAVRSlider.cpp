/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVRSlider.h"

#include <iALog.h>

#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkPropCollection.h>

iAVRSlider::iAVRSlider(vtkRenderer* ren, vtkRenderWindowInteractor* interactor) : m_renderer(ren), m_interactor(interactor)
{
	m_sliderRep = vtkSmartPointer<vtkSliderRepresentation3D>::New();
	m_sliderWidget = vtkSmartPointer<vtkSliderWidget>::New();

	m_sliderWidget->SetInteractor(m_interactor);
	m_sliderWidget->SetRepresentation(m_sliderRep);
	m_sliderWidget->EnabledOn();
	//m_interactor->Start();

	m_sliderRep->SetTitleHeight(m_sliderRep->GetTitleHeight() / 3);
	m_sliderLength = 200;
	m_visible = false;
}

void iAVRSlider::createSlider(double minValue, double maxValue, QString title)
{
	m_sliderRep->SetMinimumValue(minValue);
	m_sliderRep->SetMaximumValue(maxValue);
	m_sliderRep->SetTitleText(title.toUtf8());
	m_sliderRep->SetSliderLength(0.076);
	m_sliderRep->SetSliderWidth(0.06);
	m_sliderRep->SetEndCapLength(0.06);
	//m_sliderRep->PickableOn();

	m_sliderWidget->SetAnimationModeToAnimate();
	m_sliderWidget->PickingManagedOn();

}

void iAVRSlider::show()
{
	if (m_visible)
	{
		return;
	}
	m_sliderWidget->EnabledOn();
	m_visible = true;
}

void iAVRSlider::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_sliderWidget->EnabledOff();
	m_visible = false;
}

vtkSmartPointer<vtkProp> iAVRSlider::getSlider()
{
	vtkSmartPointer<vtkPropCollection> propColl = vtkSmartPointer<vtkPropCollection>::New();
	m_sliderRep->GetActors(propColl);
	propColl->InitTraversal();

	return propColl->GetNextProp();
}

void iAVRSlider::setSliderLength(double length)
{
	m_sliderLength = length;
}

void iAVRSlider::setPosition(double x, double y, double z)
{
	// half length
	double shift = m_sliderLength / 2.0;

	m_sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
	m_sliderRep->GetPoint1Coordinate()->SetValue(x - shift, y, z);
	m_sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
	m_sliderRep->GetPoint2Coordinate()->SetValue(x + shift, y, z);
}

void iAVRSlider::setTitel(QString title)
{
	m_sliderRep->SetTitleText(title.toUtf8());
}

void iAVRSlider::setValue(double val)
{
	m_sliderRep->SetValue(val);
}

double iAVRSlider::getValue()
{
	return m_sliderRep->GetValue();
}