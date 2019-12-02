#pragma once
#include <vtkSmartPointer.h>
#include <QMap>

class vtkPolyDataAlgorithm; 
class iAProgress; 
class QString; 
class QVariant; 

class TriangulationFilter

{	
public:

	vtkSmartPointer<vtkPolyDataAlgorithm> pointsDecimation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
		iAProgress* Progress)


};

