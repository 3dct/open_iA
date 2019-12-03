#pragma once
#include <vtkSmartPointer.h>
class vtkPolyDataMapper;
class vtkActor; 
class vtkPolyDataAlgorithm;
class vtkOpenGLRenderer;

class PolyDataObject
{

public:

	PolyDataObject(); 


	vtkSmartPointer<vtkPolyDataMapper> ObjMapper() const { return objMapper; }
	vtkSmartPointer<vtkActor> ObjActor() const { return objActor; }
	vtkSmartPointer<vtkPolyDataAlgorithm> ObjPoly() const { return objPoly; }
	void ObjPoly(vtkSmartPointer<vtkPolyDataAlgorithm> val) { objPoly = val; }

	void addToRenderer(vtkOpenGLRenderer * render);

private:

	void initPointers();

	vtkSmartPointer<vtkPolyDataMapper> objMapper;
	vtkSmartPointer<vtkActor> objActor;
	vtkSmartPointer<vtkPolyDataAlgorithm> objPoly;
};

