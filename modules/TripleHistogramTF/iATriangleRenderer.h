#pragma once
#include "iAModuleInterface.h"
#include "dlg_TripleHistogramTF.h"

class iATriangleRenderer
{
public:
	virtual ~iATriangleRenderer() {}
	virtual void setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3, BarycentricTriangle triangle) = 0;
	virtual void setTriangle(BarycentricTriangle triangle) = 0;
	virtual void paintHelper(QPainter &p) = 0;
};