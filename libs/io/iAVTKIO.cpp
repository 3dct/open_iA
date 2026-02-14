#include "iAVTKIO.h"

#include "iAITKIO.h"

#include "iAConnector.h"

#include <vtkImageData.h>

vtkSmartPointer<vtkImageData> readImage(QString const& filename)
{
	auto result = vtkSmartPointer<vtkImageData>::New();
	iAConnector con;
	iAITKIO::PixelType pixelType;
	iAITKIO::ScalarType scalarType;
	iAITKIO::ImagePointer img = iAITKIO::readFile(filename, pixelType, scalarType, true);
	//assert(pixelType == iAITKIO::PixelType::SCALAR);
	con.setImage(img);
	// only works with deep copy, not with returning vtkImage
	// assumption: ITK smart pointer goes out of scope, deletes image, and
	// invalidates "linked" vtk image
	result->DeepCopy(con.vtkImage());
	return result;
}

void storeImage(vtkSmartPointer<vtkImageData> img, QString const & filename, bool useCompression, iAProgress const* progress)
{
	iAConnector con;
	con.setImage(img);
	iAITKIO::writeFile(filename, con.itkImage(), con.itkScalarType(), useCompression, progress);
}

