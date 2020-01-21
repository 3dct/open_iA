#pragma once
#include <vtkSmartPointer.h>
#include <QMap>

class vtkPolyDataAlgorithm;
class iAProgress;
class QString;
class QVariant;
class vtkImageData;
class vtkCleanPolyData;

class TriangulationFilter

{
public:

	TriangulationFilter();

	vtkSmartPointer<vtkPolyDataAlgorithm> pointsDecimation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
		iAProgress* Progress);

	vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilterParametrisation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkImageData> imgData,
		iAProgress* Progress);

	//alpha = 0 convex hull, alpha > 0 concave hull
	//vtkSmartPointer<vtkPolyDataAlgorithm> performDelaunay(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkCleanPolyData> aSurfaceFilter, double alpha, iAProgress* progress);
	vtkSmartPointer<vtkPolyDataAlgorithm> performDelaunay(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkCleanPolyData> aSurfaceFilter, double alpha, double offset, double tolererance, iAProgress* progress);
	vtkSmartPointer<vtkPolyDataAlgorithm> Smoothing(vtkSmartPointer<vtkPolyDataAlgorithm> algo);

};

