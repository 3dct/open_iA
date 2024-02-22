// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QString>

class vtkProp;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkSliderRepresentation3D;
class vtkSliderWidget;

//class vtkSliderCallback : public vtkCommand
//{
//public:
//	static vtkSliderCallback* New()
//	{
//		return new vtkSliderCallback;
//	}
//
//	virtual void Execute(vtkObject* caller, unsigned long e, void*)
//	{
//
//		vtkSliderWidget* sliderWidget =
//			reinterpret_cast<vtkSliderWidget*>(caller);
//
//		//LOG(lvlDebug,QString("SLIDER WORKS and has Val: %1").arg(static_cast<vtkSliderRepresentation*>(sliderWidget->GetRepresentation())->GetValue()));
//		LOG(lvlDebug,QString("SLIDER WORKS with event: %1").arg(e));
//
//	}
//
//	vtkSliderCallback() {}
//
//};

//! Creates 3D Sliders in the VR Environment
class iAVRSlider
{
public:
	iAVRSlider(vtkRenderer* ren, vtkRenderWindowInteractor* interactor);
	void createSlider(double minValue, double maxValue, QString const& title = "Slider");
	void show();
	void hide();
	vtkSmartPointer<vtkProp> getSlider();
	void setSliderLength(double length);
	void setPosition(double x, double y, double z);
	void setOrientation(double y);
	void setTitel(QString const& title);
	void setValue(double val);
	double getValue();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
	vtkSmartPointer<vtkSliderRepresentation3D> m_sliderRep;
	vtkSmartPointer<vtkSliderWidget> m_sliderWidget;
	bool m_visible;
	double m_sliderLength;
};
