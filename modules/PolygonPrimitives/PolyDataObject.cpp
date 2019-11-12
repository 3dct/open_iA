#include "PolyDataObject.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkOpenGLRenderer.h"

PolyDataObject::PolyDataObject()
{
	initPointers(); 
}

void PolyDataObject::addToRenderer(vtkOpenGLRenderer* render)
{
	objMapper->SetInputConnection(objPoly->GetOutputPort()); 
	objActor->SetMapper(objMapper);
	render->AddActor(objActor); 

}

void PolyDataObject::initPointers()
{

	this->objMapper =  vtkSmartPointer<vtkPolyDataMapper>::New(); 
	this->objActor = vtkSmartPointer<vtkActor>::New(); 
	this->objPoly = vtkSmartPointer<vtkPolyDataAlgorithm>::New(); 
}
