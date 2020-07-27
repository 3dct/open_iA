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
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkSliderWidget.h>
#include <vtkSliderRepresentation3D.h>
#include <vtkRenderWindowInteractor.h>

#include <QString>

//! Creates 3D Sliders in the VR Environment
class iAVRSlider
{
public:
	iAVRSlider(vtkRenderer* ren, vtkRenderWindowInteractor* interactor);
	void createSlider(double minValue, double maxValue, QString title = "Slider");
	void show();
	void hide();
	void setPosition(double x, double y, double z);
	void setTitel(QString title);
	void setValue(double val);
	double getValue();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
	vtkSmartPointer<vtkSliderRepresentation3D> m_sliderRep;
	vtkSmartPointer<vtkSliderWidget> m_sliderWidget;
	bool m_visible;
};
