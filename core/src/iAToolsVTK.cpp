#include "iAToolsVTK.h"

#include <vtkImageData.h>

#include "iAConnector.h"
#include "iAITKIO.h"

void DeepCopy(vtkSmartPointer<vtkImageData> input, vtkSmartPointer<vtkImageData> output)
{
	output->DeepCopy(input);
}

vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int dimensions[3], double spacing[3])
{
	vtkSmartPointer<vtkImageData> result = vtkSmartPointer<vtkImageData>::New();
	result->SetDimensions(dimensions);
	result->AllocateScalars(vtkType, 1);
	result->SetSpacing(spacing);
	return result;
}

vtkSmartPointer<vtkImageData> AllocateImage(vtkSmartPointer<vtkImageData> img)
{
	return AllocateImage(img->GetScalarType(), img->GetDimensions(), img->GetSpacing());
}


void StoreImage(vtkSmartPointer<vtkImageData> image, QString const & filename, bool useCompression)
{
	iAConnector con;
	con.SetImage(image);
	iAITKIO::ScalarPixelType pixelType = con.GetITKScalarPixelType();
	iAITKIO::writeFile(filename, con.GetITKImage(), pixelType, useCompression);
}

vtkSmartPointer<vtkImageData> ReadImage(QString const & filename, bool releaseFlag)
{
	iAConnector con;
	iAITKIO::ScalarPixelType pixelType;
	iAITKIO::ImagePointer img = iAITKIO::readFile(filename, pixelType, releaseFlag);
	con.SetImage(img);
	return con.GetVTKImage();
}
