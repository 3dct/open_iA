#include "TriangulationFilter.h"
#include "vtkDecimatePro.h"
#include "vtkQuadricClustering.h"
#include "iAProgress.h"
#include <iAConsole.h>
#include "vtkImageData.h"
#include "vtkMarchingCubes.h"
#include "vtkFlyingEdges3D.h"
#include "vtkDelaunay3D.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkCleanPolyData.h"


#include <QVariant>
#include "vtkSmoothPolyDataFilter.h"
#include "vtkPolyDataNormals.h"
//#include "vtkLinearSubdivisionFilter.h"
#include "vtkButterflySubdivisionFilter.h"


TriangulationFilter::TriangulationFilter()
{

}

vtkSmartPointer<vtkPolyDataAlgorithm> TriangulationFilter::pointsDecimation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
	iAProgress* Progress) 
{
	if (!surfaceFilter) { DEBUG_LOG("Surface filter is null") return nullptr; }


	if (parameters["Simplification Algorithm"].toString() == "Decimate Pro")
	{
		auto decimatePro = vtkSmartPointer<vtkDecimatePro>::New();
		Progress->observe(decimatePro);
		decimatePro->SetTargetReduction(parameters["Decimation Target"].toDouble());
		decimatePro->SetPreserveTopology(parameters["Preserve Topology"].toBool());

		//to be removed`???
		//decimatePro->SetSplitAngle(10);
		decimatePro->SetSplitting(parameters["Splitting"].toBool());
		decimatePro->SetBoundaryVertexDeletion(parameters["Boundary Vertex Deletion"].toBool());
		decimatePro->SetInputConnection(surfaceFilter->GetOutputPort());

		return decimatePro;
	}
	else if (parameters["Simplification Algorithm"].toString() == "Quadric Clustering")
	{
		auto quadricClustering = vtkSmartPointer<vtkQuadricClustering>::New();
		Progress->observe(quadricClustering);
		quadricClustering->SetNumberOfXDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetNumberOfYDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetNumberOfZDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetInputConnection(surfaceFilter->GetOutputPort());

		return quadricClustering;

	}

}

vtkSmartPointer<vtkPolyDataAlgorithm> TriangulationFilter::surfaceFilterParametrisation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkImageData> imgData, iAProgress* Progress)
{
	if (!imgData) {
		DEBUG_LOG("input Image is null");
		return nullptr; 
	}

	if (parameters["Algorithm"].toString() == "Marching Cubes")
	{
			auto marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
			Progress->observe(marchingCubes);
			
			marchingCubes->SetInputData(imgData/*->GetPointer()*/);
			marchingCubes->ComputeNormalsOn();
			marchingCubes->ComputeGradientsOn();
			marchingCubes->ComputeScalarsOn();
			marchingCubes->SetValue(0, parameters["Iso value"].toDouble());
			return marchingCubes;
	}
	else
	{
			auto flyingEdges = vtkSmartPointer<vtkFlyingEdges3D>::New();
			Progress->observe(flyingEdges);
			flyingEdges->SetInputData(imgData/*.GetPointer()*/);
			flyingEdges->SetNumberOfContours(1);
			flyingEdges->SetValue(0, parameters["Iso value"].toDouble());
			flyingEdges->ComputeNormalsOn();
			flyingEdges->ComputeGradientsOn();
			flyingEdges->ComputeScalarsOn();
			flyingEdges->SetArrayComponent(0);
			return flyingEdges;
	}
	


	//return vtkSmartPointer<vtkPolyDataAlgorithm>();
}



vtkSmartPointer<vtkPolyDataAlgorithm> TriangulationFilter::performDelaunay(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkCleanPolyData> aSurfaceFilter,double alpha, double offset, double tolererance, iAProgress* progress)
{
	if (!aSurfaceFilter) return nullptr; 

	auto delaunay3D = vtkSmartPointer<vtkDelaunay3D>::New();
	delaunay3D->SetInputConnection(aSurfaceFilter->GetOutputPort());
	delaunay3D->SetAlpha(alpha); 
	delaunay3D->SetOffset(offset); 
	delaunay3D->SetTolerance(tolererance); 
	delaunay3D->SetAlphaTris(true); 
	delaunay3D->Update();

	auto datasetSurfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
	datasetSurfaceFilter->SetInputConnection(delaunay3D->GetOutputPort());//delaunay3D->GetOutput());
	datasetSurfaceFilter->Update();

	return datasetSurfaceFilter; 

}

vtkSmartPointer<vtkPolyDataAlgorithm> TriangulationFilter::Smoothing(vtkSmartPointer<vtkPolyDataAlgorithm> algo)
{
	/*vtkSmartPointer<vtkButterflySubdivisionFilter > filter = vtkSmartPointer<vtkButterflySubdivisionFilter >::New();
	filter->SetInputConnection(algo->GetOutputPort());
	filter->SetNumberOfSubdivisions(3);
	filter->Update();*/

	vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter =
		vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
	smoothFilter->SetInputConnection(algo->GetOutputPort());
	smoothFilter->SetNumberOfIterations(500);
	smoothFilter->SetRelaxationFactor(0.1);
	smoothFilter->FeatureEdgeSmoothingOff();
	smoothFilter->BoundarySmoothingOn();
	smoothFilter->Update();

	// Update normals on newly smoothed polydata
	vtkSmartPointer<vtkPolyDataNormals> normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
	normalGenerator->SetInputConnection(smoothFilter->GetOutputPort());
	normalGenerator->ComputePointNormalsOn();
	normalGenerator->ComputeCellNormalsOn();
	normalGenerator->SetFeatureAngle(30);
	normalGenerator->SplittingOn();
	normalGenerator->FlipNormalsOn(); 
	normalGenerator->Update();

	/*vtkSmartPointer<vtkLoopSubdivisionFilter> filter = vtkSmartPointer<vtkLoopSubdivisionFilter>::New();
	filter->SetInputConnection(normalGenerator->GetOutputPort());
	filter->Update();*/

	return normalGenerator;
}
