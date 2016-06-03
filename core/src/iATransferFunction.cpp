#include "iATransferFunction.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

vtkColorTransferFunction* GetDefaultColorTransferFunction(vtkSmartPointer<vtkImageData> imageData)
{
	vtkColorTransferFunction* cTF = vtkColorTransferFunction::New();
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(imageData->GetScalarRange()[0], 0.0, 0.0, 0.0);
	cTF->AddRGBPoint(imageData->GetScalarRange()[1], 1.0, 1.0, 1.0);
	cTF->Build();
	return cTF;
}

vtkPiecewiseFunction* GetDefaultPiecewiseFunction(vtkSmartPointer<vtkImageData> imageData)
{
	vtkPiecewiseFunction* pWF = vtkPiecewiseFunction::New();
	pWF->RemoveAllPoints();
	pWF->AddPoint(imageData->GetScalarRange()[0], 0.0);
	pWF->AddPoint(imageData->GetScalarRange()[1], 1.0);
	return pWF;
}