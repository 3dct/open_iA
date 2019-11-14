#include "PolyGen.h"
#include <vtkLineSource.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkOpenGLRenderer.h>
#include "vtkProperty.h"
#include <QColor>


vtkSmartPointer<vtkLineSource> PolyGen::createObject(double x1, double y1, double z1, double x2, double y2, double z2)
{
	vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New(); 
	lineSource->SetPoint1(x1, y1, z1);
	lineSource->SetPoint2(x2, y2, z2); 

	return lineSource; 

}

vtkSmartPointer<vtkSphereSource> PolyGen::createObject(double x, double y, double z, double radius)
{
	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New(); 
	sphereSource->SetCenter(x, y, z);
	sphereSource->SetRadius(radius);

	return sphereSource; 

}

void PolyGen::createAndRenderObject(vtkOpenGLRenderer* renderer, double x1, double y1, double z1, double x2, double y2, double z2, color acolor)
{
	vtkSmartPointer<vtkLineSource> aLine = nullptr;
	aLine = this->createObject(x1, y1, z1, x2, y2, z2);

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New(); 
	if (!aLine) throw (std::invalid_argument("values cannot be generated"));
	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New(); 
	
	auto lineProp = actor->GetProperty(); 
	lineProp->SetLineWidth(5.0); 

	switch(acolor)
	{
	
	case color::red: lineProp->SetColor(1, 0, 0); break;
	case color::blue: lineProp->SetColor(0, 1, 0); break;
	case color::green: lineProp->SetColor(0, 0, 1); break;

	default: lineProp->SetColor(1, 0, 0); break;
	
	
	}

	actor->SetMapper(mapper);

	renderer->AddActor(actor);
	

	
}
