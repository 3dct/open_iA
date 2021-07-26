#include "iACompTableInteractorStyle.h"
#include <vtkObjectFactory.h>  //for macro!

//Debug
#include "iALog.h"
#include "iACompHistogramVis.h"

//VTK
#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vtkCamera.h>


vtkStandardNewMacro(iACompTableInteractorStyle);

iACompTableInteractorStyle::iACompTableInteractorStyle() : 
	m_main(nullptr), 
	m_zoomLevel(1), 
	m_zoomOn(true)
{
}

void iACompTableInteractorStyle::setIACompHistogramVis(iACompHistogramVis* main)
{
	m_main = main;
}

void iACompTableInteractorStyle::OnLeftButtonDown()
{
}
void iACompTableInteractorStyle::OnLeftButtonUp()
{
}

void iACompTableInteractorStyle::OnMouseMove()
{
}

void iACompTableInteractorStyle::OnMiddleButtonDown()
{
}
void iACompTableInteractorStyle::OnRightButtonDown()
{
}

void iACompTableInteractorStyle::OnMouseWheelForward()
{
	//camera zoom in
	if (this->GetInteractor()->GetControlKey())
	{
		generalZoomIn();
		return;
	}
}

void iACompTableInteractorStyle::OnMouseWheelBackward()
{
	//camera zoom in
	if (this->GetInteractor()->GetControlKey())
	{
		generalZoomOut();
		return;
	}
}

void iACompTableInteractorStyle::OnKeyPress()
{
}
void iACompTableInteractorStyle::OnKeyRelease()
{
}

void iACompTableInteractorStyle::Pan()
{
}

void iACompTableInteractorStyle::generalZoomIn()
{
	m_main->getCamera()->Zoom(m_zoomLevel + 0.05);
	m_main->renderWidget();
}

void iACompTableInteractorStyle::generalZoomOut()
{
	m_main->getCamera()->Zoom(m_zoomLevel - 0.05);
	m_main->renderWidget();
}
