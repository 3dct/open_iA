#include "TriangulationFilter.h"
#include "vtkDecimatePro.h"
#include "vtkQuadricClustering.h"
#include "iAProgress.h"
#include <iAConsole.h>
#include "vtkImageData.h"
#include "vtkMarchingCubes.h"
#include "vtkFlyingEdges3D.h"


#include <QVariant>



TriangulationFilter::TriangulationFilter()
{

}

vtkSmartPointer<vtkPolyDataAlgorithm> TriangulationFilter::pointsDecimation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
	iAProgress* Progress) 
{
	if (parameters["Simplification Algorithm"].toString() == "Decimate Pro")
	{
		auto decimatePro = vtkSmartPointer<vtkDecimatePro>::New();
		Progress->observe(decimatePro);
		decimatePro->SetTargetReduction(parameters["Decimation Target"].toDouble());
		decimatePro->SetPreserveTopology(parameters["Preserve Topology"].toBool());
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
