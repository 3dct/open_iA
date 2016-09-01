#include "iATransferFunction.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

iASimpleTransferFunction::iASimpleTransferFunction(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf) :
	m_ctf(ctf),
	m_otf(otf)
{}

vtkColorTransferFunction * iASimpleTransferFunction::GetColorFunction()
{
	return m_ctf;
}

vtkPiecewiseFunction * iASimpleTransferFunction::GetOpacityFunction()
{
	return m_otf;
}

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
	if ( imageData->GetNumberOfScalarComponents() == 1 )
		pWF->AddPoint ( imageData->GetScalarRange()[0], 0.0 );
	else //Set range of rgb, rgba or vector pixel type images to fully opaque
		pWF->AddPoint( imageData->GetScalarRange()[0], 1.0 );
	pWF->AddPoint(imageData->GetScalarRange()[1], 1.0);
	return pWF;
}