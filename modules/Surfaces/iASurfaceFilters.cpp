// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAFilterDefault.h>
#include <iAFileUtils.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAPolyData.h>
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
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>

IAFILTER_DEFAULT_CLASS(iACleanPolyData);
IAFILTER_DEFAULT_CLASS(iAMeshComputeNormals);
IAFILTER_DEFAULT_CLASS(iADelauny3D);
IAFILTER_DEFAULT_CLASS(iAExtractSurface);
IAFILTER_DEFAULT_CLASS(iAPolyFillHoles);
IAFILTER_DEFAULT_CLASS(iASimplifyMeshDecimatePro);
IAFILTER_DEFAULT_CLASS(iASimplifyMeshQuadricClustering);
IAFILTER_DEFAULT_CLASS(iASimplifyMeshQuadricDecimation);
IAFILTER_DEFAULT_CLASS(iASmoothMeshWindowedSinc);
IAFILTER_DEFAULT_CLASS(iASmoothMeshLaplacian);
IAFILTER_DEFAULT_CLASS(iATransformPolyData);
IAFILTER_DEFAULT_CLASS(iAMeshTriangle);

namespace
{
	int precStrTovtkInt(QString const& precisionStr)
	{
		if (precisionStr == "Single") {
			return vtkAlgorithm::SINGLE_PRECISION;
		} else if (precisionStr == "Double") {
			return vtkAlgorithm::DOUBLE_PRECISION;
		} else {
			return vtkAlgorithm::DEFAULT_PRECISION;
		}
	}
	QStringList precisionOptions()
	{
		return QStringList() << "Same as input" << "Single" << "Double";
	}

#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 4, 0)
	int windowFuncStrTovtkInt(QString const& windowFunc)
	{
		if (windowFunc == "Blackman") {
			return vtkWindowedSincPolyDataFilter::BLACKMAN;
		} else if (windowFunc == "Hanning") {
			return vtkWindowedSincPolyDataFilter::HANNING;
		} else if (windowFunc == "Hamming") {
			return vtkWindowedSincPolyDataFilter::HAMMING;
		} else {
			return vtkWindowedSincPolyDataFilter::NUTTALL;
		}
	}
#endif
}

iAExtractSurface::iAExtractSurface() :
	iAFilter("Extract Surface", "Surfaces",
		"Extracts a surface along the specified iso value.<br/>"
		"A surface is extracted at the given Iso value, either using marching cubes or "
		"flying edges algorithm. The mesh is subsequently simplified using either "
		"a quadric clustering or the DecimatePro algorithm.<br/>"
		"For more information, see the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkMarchingCubes.html\">"
		"Marching Cubes Filter</a> and the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkFlyingEdges3D.html\">"
		"Flying Edges 3D Filter</a> in the VTK documentation.")
{
	QStringList AlgorithmNames;
	AlgorithmNames << "Marching Cubes" << "Flying Edges";
	addParameter("Algorithm", iAValueType::Categorical, AlgorithmNames);
	addParameter("Compute normals", iAValueType::Boolean, true);
	addParameter("Compute gradients", iAValueType::Boolean, true);
	addParameter("Compute scalars", iAValueType::Boolean, true);
	addParameter("Iso value", iAValueType::Continuous, 1);
}

void iAExtractSurface::performWork(QVariantMap const& parameters)
{
	vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter;
	if (parameters["Algorithm"].toString() == "Marching Cubes")
	{
		vtkNew<vtkMarchingCubes> marchingCubes;
		marchingCubes->SetComputeNormals(parameters["Compute normals"].toBool());
		marchingCubes->SetComputeGradients(parameters["Compute gradients"].toBool());
		marchingCubes->SetComputeScalars(parameters["Compute scalars"].toBool());
		marchingCubes->SetNumberOfContours(1);
		marchingCubes->SetValue(0, parameters["Iso value"].toDouble());
		surfaceFilter = marchingCubes;
	}
	else
	{
		vtkNew<vtkFlyingEdges3D> flyingEdges;
		flyingEdges->SetComputeNormals(parameters["Compute normals"].toBool());
		flyingEdges->SetComputeGradients(parameters["Compute gradients"].toBool());
		flyingEdges->SetComputeScalars(parameters["Compute scalars"].toBool());
		flyingEdges->SetNumberOfContours(1);
		flyingEdges->SetValue(0, parameters["Iso value"].toDouble());
		surfaceFilter = flyingEdges;
	}
	progress()->observe(surfaceFilter);
	surfaceFilter->SetInputData(imageInput(0)->vtkImage());
	surfaceFilter->Update();
	addOutput(std::make_shared<iAPolyData>(surfaceFilter->GetOutput()));
}

/*vtkSmartPointer<vtkButterflySubdivisionFilter > filter = vtkSmartPointer<vtkButterflySubdivisionFilter >::New();
filter->SetInputConnection(algo->GetOutputPort());
filter->SetNumberOfSubdivisions(3);
filter->Update();*/

/*auto filter = vtkSmartPointer<vtkLoopSubdivisionFilter>::New();
filter->SetInputConnection(normalGenerator->GetOutputPort());
filter->Update();*/



iASimplifyMeshDecimatePro::iASimplifyMeshDecimatePro() :
	iAFilter("Decimate Pro", "Surfaces/Simplify",
		"Simplify mesh using the Decimate Pro algorithm.<br/>"
		"<em>Target reduction</em> is the fraction of triangles in the mesh to remove (between 0 and 1, higher values mean more reduction)"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkDecimatePro.html\">"
		"Decimate Pro Filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Target reduction", iAValueType::Continuous, 0.9, 0.0, 1.0);
	addParameter("Preserve topology", iAValueType::Boolean, true);
	addParameter("Splitting", iAValueType::Boolean, true);
	addParameter("Boundary vertex deletion", iAValueType::Boolean, true);
	addParameter("Split angle", iAValueType::Continuous, 10);
}

void iASimplifyMeshDecimatePro::performWork(QVariantMap const& parameters)
{
	// crashes sometimes - non-manifold input? large datasets?
	vtkNew<vtkDecimatePro> vtkFilter;
	vtkFilter->SetPreserveTopology(parameters["Preserve topology"].toBool());
	vtkFilter->SetSplitting(parameters["Splitting"].toBool());
	vtkFilter->SetBoundaryVertexDeletion(parameters["Boundary vertex deletion"].toBool());
	vtkFilter->SetTargetReduction(parameters["Target reduction"].toDouble());
	vtkFilter->SetSplitAngle(parameters["Split angle"].toDouble());
	vtkFilter->SetInputData( dynamic_cast<iAPolyData*>(input(0).get())->poly() );
	vtkFilter->Update();
	addOutput(std::make_shared<iAPolyData>(vtkFilter->GetOutput()));
}



iASimplifyMeshQuadricClustering::iASimplifyMeshQuadricClustering() :
	iAFilter("Quadric Clustering", "Surfaces/Simplify",
		"Simplify mesh using quadric clustering.<br/>"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkQuadricClustering.html\">"
		"Quadric Clustering Filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Auto adjust number of divisions", iAValueType::Boolean, true);
	addParameter("Cluster divisions", iAValueType::Vector3i, variantVector({ 50, 50, 50 }));
	addParameter("Use input points", iAValueType::Boolean, false);
	addParameter("Use feature edges", iAValueType::Boolean, false);
	addParameter("Use feature points", iAValueType::Boolean, false);
	addParameter("Feature points angle", iAValueType::Continuous, 30.0, 0.0, 180.0);
	addParameter("Use internal triangles", iAValueType::Boolean, true);
	addParameter("Copy cell data", iAValueType::Boolean, false);
	addParameter("Prevent duplicate cells", iAValueType::Boolean, true);
}

void iASimplifyMeshQuadricClustering::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkQuadricClustering> vtkFilter;
	progress()->observe(vtkFilter);
	auto clusterDiv = variantToVector<int>(parameters["Cluster divisions"]);
	assert(clusterDiv.size() == 3);
	vtkFilter->SetAutoAdjustNumberOfDivisions(parameters["Auto adjust number of divisions"].toBool());
	vtkFilter->SetNumberOfXDivisions(clusterDiv[0]);
	vtkFilter->SetNumberOfYDivisions(clusterDiv[2]);
	vtkFilter->SetNumberOfZDivisions(clusterDiv[2]);
	vtkFilter->SetUseInputPoints(parameters["Use input points"].toBool());
	vtkFilter->SetUseFeatureEdges(parameters["Use feature edges"].toBool());
	vtkFilter->SetUseFeaturePoints(parameters["Use feature points"].toBool());
	vtkFilter->SetFeaturePointsAngle(parameters["Feature points angle"].toDouble());
	vtkFilter->SetUseInternalTriangles(parameters["Use internal triangles"].toBool());
	vtkFilter->SetCopyCellData(parameters["Copy cell data"].toBool());
	vtkFilter->SetPreventDuplicateCells(parameters["Prevent duplicate cells"].toBool());
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(std::make_shared<iAPolyData>(vtkFilter->GetOutput()));
}



iASimplifyMeshQuadricDecimation::iASimplifyMeshQuadricDecimation() :
	iAFilter("Quadric Decimation", "Surfaces/Simplify",
		"Simplify mesh using quadric decimation.<br/>"
		"From our experiments, this filter delivers the best results out of the mesh simplification algorithms implemented here."
		"<em>Target reduction</em> is the fraction of triangles in the mesh to remove (between 0 and 1, higher values mean more reduction)"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkQuadricDecimation.html\">"
		"Quadric Decimation Filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Target reduction", iAValueType::Continuous, 0.1, 0.0, 1.0);
	addParameter("Volume preservation", iAValueType::Boolean, false);
	addParameter("Attribute error metric", iAValueType::Boolean, false);
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 3, 0)
	addParameter("Regularize", iAValueType::Boolean, false);
	addParameter("Regularization", iAValueType::Continuous, 0.0);
#endif
	// lots of other parameters available (weights, attributes, ...)!
}

void iASimplifyMeshQuadricDecimation::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkQuadricDecimation> vtkFilter;
	progress()->observe(vtkFilter);
	vtkFilter->SetTargetReduction(parameters["Target reduction"].toDouble());
	vtkFilter->SetAttributeErrorMetric(parameters["Attribute error metric"].toBool());
	vtkFilter->SetVolumePreservation(parameters["Volume preservation"].toBool());
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 3, 0)
	vtkFilter->SetRegularize(parameters["Regularize"].toBool());
	vtkFilter->SetRegularization(parameters["Regularization"].toDouble());
#endif
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(std::make_shared<iAPolyData>(vtkFilter->GetOutput()));
}



iASmoothMeshWindowedSinc::iASmoothMeshWindowedSinc() :
	iAFilter("Windowed Sinc", "Surfaces/Smoothing",
		"Adjust point positions using a windowed sinc function interpolation kernel.<br/>"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkWindowedSincPolyDataFilter.html\">"
		"windowed sinc poly filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Number of iterations", iAValueType::Discrete, 20);
	addParameter("Pass band", iAValueType::Continuous, 0.1);
	addParameter("Normalize coordinates", iAValueType::Boolean, true);
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 4, 0)
	auto windowFunctions = QStringList() << "Nuttall" << "Blackman" << "Hanning" << "Hamming";
	addParameter("Window function", iAValueType::Categorical, windowFunctions);
#endif
	addParameter("Boundary smoothing", iAValueType::Boolean, false);
	addParameter("Feature edge smoothing", iAValueType::Boolean, false);
	addParameter("Feature angle", iAValueType::Continuous, 45.0, 0, 180);
	addParameter("Edge angle", iAValueType::Continuous, 15.0, 0, 180);
	addParameter("Non-manifold smoothing", iAValueType::Boolean, true);
}

void iASmoothMeshWindowedSinc::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkWindowedSincPolyDataFilter> vtkFilter;
	progress()->observe(vtkFilter);
	vtkFilter->SetNumberOfIterations(parameters["Number of iterations"].toInt());
	vtkFilter->SetPassBand(parameters["Pass band"].toDouble());
	vtkFilter->SetNormalizeCoordinates(parameters["Normalize coordinates"].toBool());
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 4, 0)
	vtkFilter->SetWindowFunction(windowFuncStrTovtkInt(parameters["Window function"].toString()));
#endif
	vtkFilter->SetBoundarySmoothing(parameters["Boundary smoothing"].toBool());
	vtkFilter->SetFeatureEdgeSmoothing(parameters["Feature edge smoothing"].toBool());
	vtkFilter->SetFeatureAngle(parameters["Feature angle"].toDouble());
	vtkFilter->SetEdgeAngle(parameters["Edge angle"].toDouble());
	vtkFilter->SetNonManifoldSmoothing(parameters["Non-manifold smoothing"].toBool());
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(std::make_shared<iAPolyData>(vtkFilter->GetOutput()));
}



iASmoothMeshLaplacian::iASmoothMeshLaplacian() :
	iAFilter("Laplacian", "Surfaces/Smoothing",
		"Adjust point positions using Laplacian smoothing.<br/>"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkSmoothPolyDataFilter.html#details\">"
		"vtkSmoothPolyDataFilter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Number of iterations", iAValueType::Discrete, 20, 0, std::numeric_limits<int>::max());
	addParameter("Feature edge smoothing", iAValueType::Boolean, false);
	addParameter("Boundary smoothing", iAValueType::Boolean, true);
	addParameter("Convergence", iAValueType::Continuous, 0.001, 0, 1);
	addParameter("Feature angle", iAValueType::Continuous, 45.0, 0, 180);
	addParameter("Edge angle", iAValueType::Continuous, 15.0, 0, 180);
	addParameter("Relaxation factor", iAValueType::Continuous, 0.01);
}

void iASmoothMeshLaplacian::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkSmoothPolyDataFilter> smoothFilter;
	smoothFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	smoothFilter->SetNumberOfIterations(parameters["Number of iterations"].toInt());
	smoothFilter->SetRelaxationFactor(parameters["Relaxation factor"].toDouble());
	smoothFilter->SetConvergence(parameters["Convergence"].toDouble());
	smoothFilter->SetFeatureEdgeSmoothing(parameters["Feature edge smoothing"].toBool());
	smoothFilter->SetBoundarySmoothing(parameters["Boundary smoothing"].toBool());
	smoothFilter->SetFeatureAngle(parameters["Feature angle"].toDouble());
	smoothFilter->SetEdgeAngle(parameters["Edge angle"].toDouble());
	smoothFilter->Update();
	addOutput(std::make_shared<iAPolyData>(smoothFilter->GetOutput()));
}



iAPolyFillHoles::iAPolyFillHoles() :
	iAFilter("Fill Holes", "Surfaces",
		"Fill holes in a mesh.<br/>"
		"<em>Hole size</em> is specified as a radius to the bounding circumsphere containing the hole (approximate).<br/>"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkFillHolesFilter.html\">"
		"Fill Hole filter</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Hole size", iAValueType::Continuous, 1.0);
}

void iAPolyFillHoles::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkFillHolesFilter> vtkFilter;
	progress()->observe(vtkFilter);
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->SetHoleSize(parameters["Hole size"].toDouble());
	vtkFilter->Update();
	addOutput(std::make_shared<iAPolyData>(vtkFilter->GetOutput()));
}



iADelauny3D::iADelauny3D() :
	iAFilter("Delauny 3D", "Surfaces",
		"Create a triangulation surface by means of the Delaunay3D-Algorithm for the given polygon.<br/>"
		"See <a href=\"https://vtk.org/doc/nightly/html/classvtkDelaunay3D.html\">VTK's documentation on Delaunay 3D</a>.", 0)
{
	setRequiredMeshInputs(1);
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
	vtkNew<vtkDelaunay3D> vtkFilter;
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

	vtkNew<vtkDataSetSurfaceFilter> datasetSurfaceFilter;
	datasetSurfaceFilter->SetInputConnection(vtkFilter->GetOutputPort());
	progress()->observe(datasetSurfaceFilter);
	datasetSurfaceFilter->Update();
	addOutput(std::make_shared<iAPolyData>(datasetSurfaceFilter->GetOutput()));
}



iATransformPolyData::iATransformPolyData() :
	iAFilter("Transform polydata", "Surfaces",
		"Apply a geometric transform (translate/rotate/scale) to polydata.<br/>"
		"See the <a href=\"https://vtk.org/doc/nightly/html/classvtkTransformPolyDataFilter.html\">"
		"Transform PolyData filter</a> in the VTK documentation.", 0)
{
	addParameter("Translate", iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addParameter("Rotate", iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addParameter("Scale", iAValueType::Vector3, variantVector<double>({ 1.0, 1.0, 1.0 }));
}

void iATransformPolyData::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkTransform> tr;
	std::array<double, 3> translate, rotate, scale;
	setFromVectorVariant<double>(translate, parameters["Translate"]);
	setFromVectorVariant<double>(rotate, parameters["Rotate"]);
	setFromVectorVariant<double>(scale, parameters["Scale"]);
	tr->RotateX(rotate[0]);
	tr->RotateY(rotate[1]);
	tr->RotateZ(rotate[2]);
	tr->Translate(translate.data());
	tr->Scale(scale.data());
	vtkNew<vtkTransformPolyDataFilter> vtkFilter;
	vtkFilter->SetTransform(tr);
	progress()->observe(vtkFilter);
	vtkFilter->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	vtkFilter->Update();
	addOutput(std::make_shared<iAPolyData>(vtkFilter->GetOutput()));
}



iAMeshComputeNormals::iAMeshComputeNormals() :
	iAFilter("Compute Normals", "Surfaces",
		"Compute (point and/or cell) normals for polygonal mesh.<br/>"
		"For more information, see the <a href=\"https://vtk.org/doc/nightly/html/classvtkPolyDataNormals.html#details\">"
		"vtkPolyDataNormals</a> in the VTK documentation.", 0)
{
	setRequiredMeshInputs(1);
	addParameter("Feature angle", iAValueType::Continuous, 30.0, 0, 180);
	addParameter("Splitting", iAValueType::Boolean, true);
	addParameter("Consistency", iAValueType::Boolean, true);
	addParameter("Flip normals", iAValueType::Boolean, false);
	addParameter("Compute point normals", iAValueType::Boolean, true);
	addParameter("Compute cell normals", iAValueType::Boolean, false);
	addParameter("Non-manifold traversal", iAValueType::Boolean, true);
	addParameter("Auto-orient normals", iAValueType::Boolean, false);
	addParameter("Output precision", iAValueType::Categorical, precisionOptions());
}

void iAMeshComputeNormals::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkPolyDataNormals> normalGenerator;
	normalGenerator->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	normalGenerator->SetFeatureAngle(parameters["Feature angle"].toDouble());
	normalGenerator->SetSplitting(parameters["Splitting"].toBool());
	normalGenerator->SetConsistency(parameters["Consistency"].toBool());
	normalGenerator->SetFlipNormals(parameters["Flip normals"].toBool());
	normalGenerator->SetComputePointNormals(parameters["Compute point normals"].toBool());
	normalGenerator->SetComputeCellNormals(parameters["Compute cell normals"].toBool());
	normalGenerator->SetNonManifoldTraversal(parameters["Non-manifold traversal"].toBool());
	normalGenerator->SetAutoOrientNormals(parameters["Auto-orient normals"].toBool());
	normalGenerator->SetOutputPointsPrecision(precStrTovtkInt(parameters["Output precision"].toString()));
	normalGenerator->Update();
	addOutput(std::make_shared<iAPolyData>(normalGenerator->GetOutput()));
}



iACleanPolyData::iACleanPolyData() :
	iAFilter("Clean polydata", "Surfaces",
		"Merge duplicate points, and/or remove unused points and/or remove degenerate cells from polydata.<br/>"
		"See the <a href=\"https://vtk.org/doc/nightly/html/classvtkCleanPolyData.html\">"
		"Clean PolyData filter</a> in the VTK documentation.", 0)
{
	addParameter("Point merging", iAValueType::Boolean, true);
	addParameter("Convert lines to points", iAValueType::Boolean, true);
	addParameter("Convert polys to lines", iAValueType::Boolean, true);
	addParameter("Convert strips to polys", iAValueType::Boolean, true);
	//addParameter("Piece invariant", iAValueType::Boolean, );  // only relevant for streaming?
	addParameter("Tolerance is absolute", iAValueType::Boolean, false);
	addParameter("Tolerance", iAValueType::Continuous, 0.0);
	addParameter("Absolute tolerance", iAValueType::Continuous, 1.0);
	addParameter("Output precision", iAValueType::Categorical, precisionOptions());
}

void iACleanPolyData::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkCleanPolyData> cleaner;
	cleaner->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	cleaner->SetAbsoluteTolerance(parameters["Absolute Tolerance"].toDouble());
	cleaner->SetToleranceIsAbsolute(parameters["Tolerance is absolute"].toBool());
	cleaner->SetTolerance(parameters["Tolerance"].toDouble());
	cleaner->SetConvertLinesToPoints(parameters["Convert lines to points"].toBool());
	cleaner->SetConvertPolysToLines(parameters["Convert polys to lines"].toBool());
	cleaner->SetConvertStripsToPolys(parameters["Convert strips to polys"].toBool());
	cleaner->SetPointMerging(parameters["Point merging"].toBool());
	cleaner->SetOutputPointsPrecision(precStrTovtkInt(parameters["Output precision"].toString()));
	cleaner->Update();
	addOutput(std::make_shared<iAPolyData>(cleaner->GetOutput()));
}


iAMeshTriangle::iAMeshTriangle() :
	iAFilter("Convert to triangles", "Surfaces",
		"Convert input polygons and strips to triangles.<br/>"
		"See the <a href=\"https://vtk.org/doc/nightly/html/classvtkTriangleFilter.html#details\">"
		"triangle filter</a> in the VTK documentation.", 0)
{
	//addParameter("Preserve polys", iAValueType::Boolean, false);  // available in VTK >= 9.4, not sure for what purpose
	addParameter("Pass vertices through filter", iAValueType::Boolean, true);
	addParameter("Pass lines through filter", iAValueType::Boolean, true);
	addParameter("Tolerance", iAValueType::Continuous, -1.0);
}

void iAMeshTriangle::performWork(QVariantMap const& parameters)
{
	vtkNew<vtkTriangleFilter> triangles;
	triangles->SetInputData(dynamic_cast<iAPolyData*>(input(0).get())->poly());
	//triangles->PreservePolys(parameters["Absolute Tolerance"].toDouble());
	triangles->SetPassVerts(parameters["Pass vertices through filter"].toBool());
	triangles->SetPassLines(parameters["Pass lines through filter"].toBool());
	triangles->SetTolerance(parameters["Tolerance"].toDouble());
	triangles->Update();
	addOutput(std::make_shared<iAPolyData>(triangles->GetOutput()));
}