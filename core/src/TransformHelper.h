#pragma once

#include "vtkSmartPointer.h"
//class vtkSmartPointer;
class vtkTransform;
class vtkImageReslice;

namespace transform {

	enum rotatationMode {
		x, y, z, 

	};

}

//should perform all transformation

class TransformHelper
{
public:
	TransformHelper();
	~TransformHelper();

	void ReslicerRotate(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslicer, unsigned int sliceMode, double const * center, double angle, double const *spacing);


	inline void setSpacing(double const *spacing) {
		if (!spacing)
			return;

		for (int i = 0; i < 3; i++) {
			m_spacing[i] = spacing[i]; 
		}
	}

private: 

	double m_spacing[3]; 

};

