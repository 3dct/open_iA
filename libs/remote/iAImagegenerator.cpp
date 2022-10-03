#include "iAImagegenerator.h"
#include <vtkUnsignedCharArray.h>
#include <vtkJPEGWriter.h>
#include <vtkWindowToImageFilter.h>

vtkUnsignedCharArray* iAImagegenerator::createImage(vtkRenderWindow* window, int quality)
{

	vtkNew<vtkWindowToImageFilter> w2if;
	w2if->SetInput(window);
	w2if->Update();

	vtkNew<vtkJPEGWriter> writer;
	//writer->SetFileName("TestEarthSource->png");
	writer->SetInputConnection(w2if->GetOutputPort());
	writer->WriteToMemoryOn();
	return writer->GetResult();
}
