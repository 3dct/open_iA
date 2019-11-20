#pragma once

#include <vtkSmartPointer.h>
#include <vtkPolyDataAlgorithm.h>

class vtkCubeSource; 
class vtkSphereSource; 
class vtkLineSource; 
class QLineF;
class vtkOpenGLRenderer;

enum color{red, blue, green};

class PolySphere{
	PolySphere(double Centerx, double Centery, double Centerz, double radiusVal) :x(Centerx), y(Centery), z(Centerz), radius(radiusVal) {
	
	
	};


	inline double getX() const{
		return this->x; 
	} 

	inline double getY() const {
		return this->y;
	}

	inline double getZ() const {
		return this->z;
	}

	inline double getRaduis() const {
		return this->radius;
	}

private:
	PolySphere(const PolySphere& other) = delete ;

	double x, y, z;
	double radius;

};



class PolyGen
{
public:
	vtkSmartPointer<vtkSphereSource> createObject(double x, double y, double z, double radius); 
	vtkSmartPointer<vtkLineSource> createObject(double x1, double y1, double z1, double x2, double y2, double z2 );
	vtkSmartPointer<vtkCubeSource> createCube(double x11, double y1, double z1, double x2, double y2, double z2) ; 


	vtkSmartPointer<vtkCubeSource> createCube(double center[3], double xdim, double ydim, double zdim);
	// void createAndRenderLine(vtkOpenGLRenderer* renderer, double x1, double y1, double z1, double x2, double y2, double z2, color acolor);
	 void createAndRenderLine(vtkOpenGLRenderer* renderer, double x1, double y1, double z1, double x2, double y2, double z2, double lnWithd, color acolor);
	 void createAndRenderSphere(vtkOpenGLRenderer* renderer, double xm, double ym, double zm, double radius, color acolor);

};

