// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iADataSet.h>
#include <iAFilterDefault.h>
#include <iAFileUtils.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iAValueTypeVectorHelpers.h>

#include <vtkCleanPolyData.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDecimatePro.h>
#include <vtkDelaunay3D.h>
#include <vtkFillHolesFilter.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataNormals.h>
#include <vtkQuadricDecimation.h>
#include <vtkQuadricClustering.h>
#include <vtkSmartPointer.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>

IAFILTER_DEFAULT_CLASS(iAExtractSurface);
IAFILTER_DEFAULT_CLASS(iADelauny3D);
IAFILTER_DEFAULT_CLASS(iAFillHoles);
IAFILTER_DEFAULT_CLASS(iASimplifyMeshDecimatePro);
IAFILTER_DEFAULT_CLASS(iASimplifyMeshQuadricClustering);
IAFILTER_DEFAULT_CLASS(iASimplifyMeshQuadricDecimation);
IAFILTER_DEFAULT_CLASS(iASmoothMeshWindowedSinc);
IAFILTER_DEFAULT_CLASS(iATriangulation);

namespace
{
	vtkSmartPointer<vtkPolyDataAlgorithm> createDecimation(QVariantMap const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
		iAProgress* Progress)
	{
		if (!surfaceFilter)
		{
			LOG(lvlError, "Surface filter is null");
			return nullptr;
		}

		QString simplifyAlgoName = parameters["Simplification Algorithm"].toString();
		vtkSmartPointer<vtkPolyDataAlgorithm> result;
		if (simplifyAlgoName == "Decimate Pro")
		{
			// crashes sometimes - non-manifold input? large datasets?
			auto decimatePro = vtkSmartPointer<vtkDecimatePro>::New();
			decimatePro->SetTargetReduction(parameters["Decimation Target"].toDouble());
			decimatePro->SetPreserveTopology(parameters["Preserve Topology"].toBool());

			//to be removed`???
			//decimatePro->SetSplitAngle(10);
			decimatePro->SetSplitting(parameters["Splitting"].toBool());
			decimatePro->SetBoundaryVertexDeletion(parameters["Boundary Vertex Deletion"].toBool());
			result = decimatePro;
		}
		else if (simplifyAlgoName == "Quadric Clustering")
		{
			auto quadricClustering = vtkSmartPointer<vtkQuadricClustering>::New();
			Progress->observe(quadricClustering);
			quadricClustering->SetNumberOfXDivisions(parameters["Cluster divisions"].toUInt());
			quadricClustering->SetNumberOfYDivisions(parameters["Cluster divisions"].toUInt());
			quadricClustering->SetNumberOfZDivisions(parameters["Cluster divisions"].toUInt());
			quadricClustering->SetInputConnection(surfaceFilter->GetOutputPort());
			result = quadricClustering;
		}
		else if (simplifyAlgoName == "Windowed Sinc")
		{
			auto windowedSinc = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
			windowedSinc->SetNumberOfIterations  (parameters["Number of Iterations"].toInt());     // 15
			windowedSinc->SetBoundarySmoothing   (parameters["Boundary Smoothing"].toBool());      // false
			windowedSinc->SetFeatureEdgeSmoothing(parameters["Feature Edge Smoothing"].toBool());  // false
			windowedSinc->SetFeatureAngle        (parameters["Feature Angle"].toDouble());         // 120
			windowedSinc->SetPassBand            (parameters["Pass Band"].toDouble());             // .001
			windowedSinc->SetNonManifoldSmoothing(parameters["Non-Manifold Smoothing"].toBool());  // true
			windowedSinc->SetNormalizeCoordinates(parameters["Normalize Coordinates"].toBool());   // true
			result = windowedSinc;
		}
		else
		{
			LOG(lvlError, QString("Unknown simplification algorithm '%1'").arg(simplifyAlgoName));
			return nullptr;
		}
		Progress->observe(result);
		result->SetInputConnection(surfaceFilter->GetOutputPort());
		return result;
	}

	vtkSmartPointer<vtkPolyDataAlgorithm> createSurfaceFilter(QVariantMap const& parameters, vtkSmartPointer<vtkImageData> imgData, iAProgress* Progress)
	{
		if (!imgData)
		{
			LOG(lvlError, "input Image is null");
			return nullptr;
		}
		vtkSmartPointer<vtkPolyDataAlgorithm> result;
		if (parameters["Algorithm"].toString() == "Marching Cubes")
		{
			auto marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
			marchingCubes->ComputeNormalsOn();
			marchingCubes->ComputeGradientsOn();
			marchingCubes->ComputeScalarsOn();
			marchingCubes->SetValue(0, parameters["Iso value"].toDouble());
			result = marchingCubes;
		}
		else
		{
			auto flyingEdges = vtkSmartPointer<vtkFlyingEdges3D>::New();
			flyingEdges->SetNumberOfContours(1);
			flyingEdges->SetValue(0, parameters["Iso value"].toDouble());
			flyingEdges->ComputeNormalsOn();
			flyingEdges->ComputeGradientsOn();
			flyingEdges->ComputeScalarsOn();
			flyingEdges->SetArrayComponent(0);
			result = flyingEdges;
		}
		Progress->observe(result);
		result->SetInputData(imgData);
		return result;
	}

	vtkSmartPointer<vtkPolyDataAlgorithm> createTriangulation(
		QVariantMap const& /*parameters*/, vtkSmartPointer<vtkCleanPolyData> aSurfaceFilter,
		double alpha, double offset, double tolererance, iAProgress* progress)
	{
		if (!aSurfaceFilter)
		{
			return nullptr;
		}

		auto delaunay3D = vtkSmartPointer<vtkDelaunay3D>::New();
		delaunay3D->SetInputConnection(aSurfaceFilter->GetOutputPort());
		delaunay3D->SetAlpha(alpha);
		delaunay3D->SetOffset(offset);
		delaunay3D->SetTolerance(tolererance);
		delaunay3D->SetAlphaTris(true);
		progress->observe(delaunay3D);
		delaunay3D->Update();

		auto datasetSurfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
		datasetSurfaceFilter->SetInputConnection(delaunay3D->GetOutputPort());//delaunay3D->GetOutput());
		progress->observe(datasetSurfaceFilter);
		datasetSurfaceFilter->Update();

		return datasetSurfaceFilter;
	}

	vtkSmartPointer<vtkPolyDataAlgorithm> createSmoothing(vtkSmartPointer<vtkPolyDataAlgorithm> algo)
	{
		/*vtkSmartPointer<vtkButterflySubdivisionFilter > filter = vtkSmartPointer<vtkButterflySubdivisionFilter >::New();
		filter->SetInputConnection(algo->GetOutputPort());
		filter->SetNumberOfSubdivisions(3);
		filter->Update();*/

		auto smoothFilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
		smoothFilter->SetInputConnection(algo->GetOutputPort());
		smoothFilter->SetNumberOfIterations(500);
		smoothFilter->SetRelaxationFactor(0.1);
		smoothFilter->FeatureEdgeSmoothingOff();
		smoothFilter->BoundarySmoothingOn();
		smoothFilter->Update();

		// Update normals on newly smoothed polydata
		auto normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
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
}

void iAExtractSurface::performWork(QVariantMap const & parameters)
{
	auto surfaceFilter = createSurfaceFilter(parameters, imageInput(0)->vtkImage(), progress());
	if (!surfaceFilter)
	{
		LOG(lvlError, "Generated surface filter is null");
		return;
	}
	if (parameters["Simplification Algorithm"].toString() == "None")
	{
		surfaceFilter->Update();
		addOutput(surfaceFilter->GetOutput());
	}
	else
	{
		auto simplifyFilter = createDecimation(parameters, surfaceFilter, progress());
		simplifyFilter->Update();
		addOutput(simplifyFilter->GetOutput());
	}
}

iAExtractSurface::iAExtractSurface() :
	iAFilter("Extract Surface", "Extract Surface",
		"Extracts a surface along the specified iso value.<br/>"
		"A surface is extracted at the given Iso value, either using marching cubes or "
		"flying edges algorithm. The mesh is subsequently simplified using either "
		"a quadric clustering or the DecimatePro algorithm.<br/>"
		"For more information, see the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkMarchingCubes.html\">"
		"Marching Cubes Filter</a>, the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkFlyingEdges3D.html\">"
		"Flying Edges 3D Filter</a>, the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkDecimatePro.html\">"
		"Decimate Pro Filter</a>, the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkWindowedSincPolyDataFilter.html\">"
		"Windowed Sinc Poly Data Filter</a>, and the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkQuadricClustering.html\">"
		"Quadric Clustering Filter</a> in the VTK documentation.")
{
	QStringList AlgorithmNames;
	AlgorithmNames << "Marching Cubes" << "Flying Edges";
	addParameter("Extraction Algorithm", iAValueType::Categorical, AlgorithmNames);
	addParameter("Iso value", iAValueType::Continuous, 1);
	QStringList SimplificationAlgorithms;
	SimplificationAlgorithms << "Quadric Clustering" << "Decimate Pro" << "Windowed Sinc" << "None";
	addParameter("Simplification Algorithm", iAValueType::Categorical, SimplificationAlgorithms);

	// Decimate Pro parameters:
	addParameter("Preserve Topology", iAValueType::Boolean, true);
	addParameter("Splitting", iAValueType::Boolean, true);
	addParameter("Boundary Vertex Deletion", iAValueType::Boolean, true);
	addParameter("Decimation Target", iAValueType::Continuous, 0.9);

	// Quadric Clustering parameters:
	addParameter("Cluster divisions", iAValueType::Discrete, 128);

	// Windowed Sinc parameters:
	addParameter("Number of Iterations", iAValueType::Discrete, 15);
	addParameter("Boundary Smoothing", iAValueType::Boolean, false);
	addParameter("Feature Edge Smoothing", iAValueType::Boolean, false);
	addParameter("Feature Angle", iAValueType::Continuous, 120.0);
	addParameter("Pass Band", iAValueType::Continuous, 0.001);
	addParameter("Non-Manifold Smoothing", iAValueType::Boolean, true);
	addParameter("Normalize Coordinates", iAValueType::Boolean, true);
}

void iATriangulation::performWork(QVariantMap const& parameters) {

	auto surfaceFilter = createSurfaceFilter(parameters, imageInput(0)->vtkImage(), progress());
	if (!surfaceFilter)
	{
		LOG(lvlError, "Generated surface filter is null");
		return;
	}

	vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
	if (parameters["Simplification Algorithm"].toString() == "None")
	{
		cleaner->SetInputConnection(surfaceFilter->GetOutputPort());
	}
	else
	{
		auto simplifyFilter = createDecimation(parameters, surfaceFilter, progress());
		cleaner->SetInputConnection(simplifyFilter->GetOutputPort());
	}

	auto cleanTol = parameters["CleanTolerance"].toDouble();
	cleaner->SetTolerance(cleanTol);
	cleaner->Update();
	double alpha = parameters["Alpha"].toDouble();
	double offset = parameters["Offset"].toDouble();
	double tolerance = parameters["Tolerance"].toDouble();

	auto delaunyFilter = createTriangulation(parameters, cleaner, alpha,offset,tolerance, progress());

	auto smoothing = createSmoothing(delaunyFilter);

	addOutput(smoothing->GetOutput());
}


iATriangulation::iATriangulation() :
	iAFilter("Surface triangulation", "Extract Surface",
		"Extracts a surface along the specified iso value.<br/>"
		"A surface is extracted at the given Iso value, either using marching cubes or "
		"flying edges algorithm. The mesh is subsequently simplified using either "
		"a quadric clustering or the DecimatePro algorithm.<br/>"
		"Based on the points of the extracted surface, a triangulation surface by means of the Delaunay3D-Algorithm will be created"
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkDelaunay3D.html\"> "
		"cf. Delaunay3D </a>"
		"For more information, see the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkMarchingCubes.html\">"
		"Marching Cubes Filter</a>, the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkFlyingEdges3D.html\">"
		"Flying Edges 3D Filter</a>, the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkDecimatePro.html\">"
		"Decimate Pro Filter</a>, and the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkQuadricClustering.html\">"
		"Quadric Clustering Filter</a> in the VTK documentation.")

{
	QStringList AlgorithmNames;
	AlgorithmNames << "Marching Cubes" << "Flying Edges";
	addParameter("Extraction Algorithm", iAValueType::Categorical, AlgorithmNames);
	addParameter("Iso value", iAValueType::Continuous, 1);
	QStringList SimplificationAlgorithms;
	SimplificationAlgorithms << "Quadric Clustering" << "Decimate Pro" << "None";
	addParameter("Simplification Algorithm", iAValueType::Categorical, SimplificationAlgorithms);
	addParameter("Preserve Topology", iAValueType::Boolean, true);
	addParameter("Splitting", iAValueType::Boolean, true);
	addParameter("Boundary Vertex Deletion", iAValueType::Boolean, true);
	addParameter("Decimation Target", iAValueType::Continuous, 0.9);
	addParameter("Cluster divisions", iAValueType::Discrete, 128);
	addParameter("Alpha", iAValueType::Continuous, 0);
	addParameter("Offset", iAValueType::Continuous, 0);
	addParameter("Tolerance", iAValueType::Continuous, 0.001);
	addParameter("CleanTolerance", iAValueType::Continuous, 0);
}



iASimplifyMeshDecimatePro::iASimplifyMeshDecimatePro() :
	iAFilter("Simplify: Decimate Pro", "Extract Surface",
		"Simplify mesh using the Decimate Pro algorithm.<br/>"
		"See the <a href=\"https://vtk.org/doc/nightly/html/classvtkDecimatePro.html\">"
		"Decimate Pro Filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Preserve Topology", iAValueType::Boolean, true);
	addParameter("Splitting", iAValueType::Boolean, true);
	addParameter("Boundary Vertex Deletion", iAValueType::Boolean, true);
	addParameter("Decimation Target", iAValueType::Continuous, 0.9);
	addParameter("Split Angle", iAValueType::Continuous, 10);
}

void iASimplifyMeshDecimatePro::performWork(QVariantMap const& parameters)
{
	// crashes sometimes - non-manifold input? large datasets?
	auto vtkFilter = vtkSmartPointer<vtkDecimatePro>::New();
	vtkFilter->SetTargetReduction(parameters["Decimation Target"].toDouble());
	vtkFilter->SetPreserveTopology(parameters["Preserve Topology"].toBool());
	vtkFilter->SetSplitAngle(parameters["Split Angle"].toDouble());
	vtkFilter->SetSplitting(parameters["Splitting"].toBool());
	vtkFilter->SetBoundaryVertexDeletion(parameters["Boundary Vertex Deletion"].toBool());
	vtkFilter->SetInputData( dynamic_cast<iAPolyData*>(input(0).get())->poly() );
	vtkFilter->Update();
	addOutput(vtkFilter->GetOutput());
}



iASimplifyMeshQuadricClustering::iASimplifyMeshQuadricClustering() :
	iAFilter("Simplify: Quadric Clustering", "Extract Surface",
		"Simplify mesh using quadric clustering.<br/>"
		"See the <a href=\"https://vtk.org/doc/nightly/html/classvtkQuadricClustering.html\">"
		"Quadric Clustering Filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Auto adjust number of divisions", iAValueType::Boolean, true);
	addParameter("Cluster divisions", iAValueType::Vector3i, variantVector({ 100, 100, 100 }));

	// lots of other parameters available!
}

void iASimplifyMeshQuadricClustering::performWork(QVariantMap const& parameters)
{
	auto vtkFilter = vtkSmartPointer<vtkQuadricClustering>::New();
	progress()->observe(vtkFilter);
	auto clusterDiv = variantToVector<int>(parameters["Cluster divisions"]);
	assert(clusterDiv.size() == 3);
	vtkFilter->SetAutoAdjustNumberOfDivisions(parameters["Auto adjust number of divisions"].toBool());
	vtkFilter->SetNumberOfXDivisions(clusterDiv[0]);
	vtkFilter->SetNumberOfYDivisions(clusterDiv[2]);
	vtkFilter->SetNumberOfZDivisions(clusterDiv[2]);
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(vtkFilter->GetOutput());
}



iASimplifyMeshQuadricDecimation::iASimplifyMeshQuadricDecimation() :
	iAFilter("Simplify: Quadric Decimation", "Extract Surface",
		"Simplify mesh using quadric decimation.<br/>"
		"See the <a href=\"https://vtk.org/doc/nightly/html/classvtkQuadricDecimation.html\">"
		"Quadric Decimation Filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Target Reduction", iAValueType::Continuous, 0.1);
	addParameter("Attribute Error Metric", iAValueType::Boolean, false);
	addParameter("Volume Preservation", iAValueType::Boolean, false);
	// should be there according to nightly doc but aren't there in 9.1.0.... added since?
	//addParameter("Regularize", iAValueType::Boolean, false);
	//addParameter("Regularization", iAValueType::Continuous, 0.0);
	
	// lots of other parameters available!
}

void iASimplifyMeshQuadricDecimation::performWork(QVariantMap const& parameters)
{
	auto vtkFilter = vtkSmartPointer<vtkQuadricDecimation>::New();
	progress()->observe(vtkFilter);
	vtkFilter->SetTargetReduction(parameters["Target Reduction"].toDouble());
	vtkFilter->SetAttributeErrorMetric(parameters["Attribute Error Metric"].toBool());
	vtkFilter->SetVolumePreservation(parameters["Volume Preservation"].toBool());
	//vtkFilter->SetRegularize(parameters["Regularize"].toBool());
	//vtkFilter->SetRegularization(parameters["Regularization"].toDouble());
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(vtkFilter->GetOutput());
}



iASmoothMeshWindowedSinc::iASmoothMeshWindowedSinc() :
	iAFilter("Mesh Smoothing (Windowed Sinc)", "Extract Surface",
		"Smooth mesh. <br/>Uses <a href=\"https://vtk.org/doc/nightly/html/classvtkWindowedSincPolyDataFilter.html\">VTK'swindowed sinc poly filter</a>.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Number of Iterations", iAValueType::Discrete, 15);
	addParameter("Boundary Smoothing", iAValueType::Boolean, false);
	addParameter("Feature Edge Smoothing", iAValueType::Boolean, false);
	addParameter("Feature Angle", iAValueType::Continuous, 120.0);
	addParameter("Pass Band", iAValueType::Continuous, 0.001);
	addParameter("Non-Manifold Smoothing", iAValueType::Boolean, true);
	addParameter("Normalize Coordinates", iAValueType::Boolean, true);
}

void iASmoothMeshWindowedSinc::performWork(QVariantMap const& parameters)
{
	auto vtkFilter = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	progress()->observe(vtkFilter);
	vtkFilter->SetNumberOfIterations(parameters["Number of Iterations"].toInt());     // 15
	vtkFilter->SetBoundarySmoothing(parameters["Boundary Smoothing"].toBool());      // false
	vtkFilter->SetFeatureEdgeSmoothing(parameters["Feature Edge Smoothing"].toBool());  // false
	vtkFilter->SetFeatureAngle(parameters["Feature Angle"].toDouble());         // 120
	vtkFilter->SetPassBand(parameters["Pass Band"].toDouble());             // .001
	vtkFilter->SetNonManifoldSmoothing(parameters["Non-Manifold Smoothing"].toBool());  // true
	vtkFilter->SetNormalizeCoordinates(parameters["Normalize Coordinates"].toBool());   // true
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(vtkFilter->GetOutput());
}

iAFillHoles::iAFillHoles() :
	iAFilter("Fill Holes", "Extract Surface",
		"Fill holes in a mesh.<br/>"
		"Hole size is specified as a radius to the bounding circumsphere containing the hole (approximate)."
		"Uses <a href=\"https://vtk.org/doc/nightly/html/classvtkFillHolesFilter.html\">VTK's Fill Hole filter</a>.", 0)
{
	addParameter("Hole size", iAValueType::Continuous, 1.0);
}

void iAFillHoles::performWork(QVariantMap const& parameters)
{
	auto vtkFilter = vtkSmartPointer<vtkFillHolesFilter>::New();
	progress()->observe(vtkFilter);
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->SetHoleSize(parameters["Hole size"].toDouble());
	vtkFilter->Update();
	addOutput(vtkFilter->GetOutput());
}

iADelauny3D::iADelauny3D() :
	iAFilter("Delauny 3D", "Extract Surface",
		"Create a triangulation surface by means of the Delaunay3D-Algorithm for the given polygon."
		"See <a href=\"https://vtk.org/doc/nightly/html/classvtkDelaunay3D.html\">VTK documentation on Delaunay3D</a>.", 0)
{
	addParameter("Alpha", iAValueType::Continuous, 0);
	addParameter("Triangles for non-zero alpha values", iAValueType::Boolean, true);
	addParameter("Tetrahedra for non-zero alpha values", iAValueType::Boolean, true);
	addParameter("Vertices for non-zero alpha values", iAValueType::Boolean, true);
	addParameter("Lines for non-zero alpha values", iAValueType::Boolean, true);
	addParameter("Offset", iAValueType::Continuous, 0);
	addParameter("Tolerance", iAValueType::Continuous, 0.001);
}

void iADelauny3D::performWork(QVariantMap const& parameters)
{
	auto vtkFilter = vtkSmartPointer<vtkDelaunay3D>::New();
	progress()->observe(vtkFilter);
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->SetAlpha(parameters["Alpha"].toDouble());
	vtkFilter->SetOffset(parameters["Offset"].toDouble());
	vtkFilter->SetTolerance(parameters["Tolerance"].toDouble());
	vtkFilter->SetAlphaTris(parameters["Triangles for non-zero alpha values"].toDouble());
	vtkFilter->SetAlphaTets(parameters["Tetrahedra for non-zero alpha values"].toDouble());
	vtkFilter->SetAlphaVerts(parameters["Vertices for non-zero alpha values"].toDouble());
	vtkFilter->SetAlphaLines(parameters["Lines for non-zero alpha values"].toDouble());
	vtkFilter->Update();
	//vtkFilter->GetOutput() // vtkUnstructuredGrid!

	auto datasetSurfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
	datasetSurfaceFilter->SetInputConnection(vtkFilter->GetOutputPort());
	progress()->observe(datasetSurfaceFilter);
	datasetSurfaceFilter->Update();
	addOutput(datasetSurfaceFilter->GetOutput());
}
