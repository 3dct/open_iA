#include "iAComp3DWidgetInteractionStyle.h"
#include <vtkObjectFactory.h> //for macro!

#include "iAComp3DWidget.h"

//vtk
#include "vtkRenderWindowInteractor.h"

vtkStandardNewMacro(iAComp3DWidgetInteractionStyle);

iAComp3DWidgetInteractionStyle::iAComp3DWidgetInteractionStyle(): 
	m_visualization(nullptr)
{

}

void iAComp3DWidgetInteractionStyle::setVisualization(iAComp3DWidget* visualization)
{
	m_visualization = visualization;
}

void iAComp3DWidgetInteractionStyle::OnLeftButtonDown()
{
	vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	if (interactor == nullptr) return;


	if (interactor->GetShiftKey())
	{
		m_visualization->resetWidget();
		m_visualization->renderWidget();
	}
}
