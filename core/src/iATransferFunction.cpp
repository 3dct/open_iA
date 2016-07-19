#include "iATransferFunction.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

vtkSmartPointer<vtkColorTransferFunction> GetDefaultColorTransferFunction(vtkSmartPointer<vtkImageData> imageData)
{
	auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(imageData->GetScalarRange()[0], 0.0, 0.0, 0.0);
	cTF->AddRGBPoint(imageData->GetScalarRange()[1], 1.0, 1.0, 1.0);
	cTF->Build();
	return cTF;
}

vtkSmartPointer<vtkPiecewiseFunction> GetDefaultPiecewiseFunction(vtkSmartPointer<vtkImageData> imageData)
{
	auto pWF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	pWF->RemoveAllPoints();
	pWF->AddPoint(imageData->GetScalarRange()[0], 0.0);
	pWF->AddPoint(imageData->GetScalarRange()[1], 1.0);
	return pWF;
}