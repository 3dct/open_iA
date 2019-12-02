#pragma once
#include <vtkSmartPointer.h>
#include <QMap>

class vtkPolyDataAlgorithm; 
class iAProgress; 
class QString; 
class QVariant; 
class vtkImageData; 

class TriangulationFilter

{	
public:

	TriangulationFilter(); 

	vtkSmartPointer<vtkPolyDataAlgorithm> pointsDecimation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
		iAProgress* Progress);

	vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilterParametrisation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkImageData> imgData,
		iAProgress* Progress);




};

