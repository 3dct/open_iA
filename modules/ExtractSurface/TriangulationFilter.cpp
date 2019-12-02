#include "TriangulationFilter.h"
#include "vtkDecimatePro.h"
#include "vtkQuadricClustering.h"
#include "iAProgress.h"

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
