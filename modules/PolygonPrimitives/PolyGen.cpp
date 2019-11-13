#include "PolyGen.h"
#include <vtkLineSource.h>
#include <vtkSphereSource.h>


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
