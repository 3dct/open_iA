/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAExtractSurfaceFilters.h"

// core
#include <iAProgress.h>

// base
#include <iAConnector.h>
#include <iALog.h>
#include <iAFileUtils.h>

//#include <vtkButterflySubdivisionFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDecimatePro.h>
#include <vtkDelaunay3D.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataNormals.h>
#include <vtkQuadricDecimation.h>
#include <vtkQuadricClustering.h>
#include <vtkSmartPointer.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkSTLWriter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkFillHolesFilter.h>

namespace
{
	vtkSmartPointer<vtkPolyDataAlgorithm> createDecimation(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter,
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

	vtkSmartPointer<vtkPolyDataAlgorithm> createSurfaceFilter(QMap<QString, QVariant> const& parameters, vtkSmartPointer<vtkImageData> imgData, iAProgress* Progress)
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
		QMap<QString, QVariant> const& /*parameters*/, vtkSmartPointer<vtkCleanPolyData> aSurfaceFilter,
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

void iAExtractSurface::performWork(QMap<QString, QVariant> const & parameters)
{
	auto surfaceFilter = createSurfaceFilter(parameters, input(0)->vtkImage(), progress());
	if (!surfaceFilter)
	{
		LOG(lvlError, "Generated surface filter is null");
		return;
	}

	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	progress()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());

	if (parameters["Simplification Algorithm"].toString() == "None")
	{
		surfaceFilter->Update();
		setPolyOutput(surfaceFilter->GetOutput());
		stlWriter->SetInputConnection(surfaceFilter->GetOutputPort());
	}
	else
	{
		auto simplifyFilter = createDecimation(parameters, surfaceFilter, progress());
		simplifyFilter->Update();
		setPolyOutput(simplifyFilter->GetOutput());
		stlWriter->SetInputConnection(simplifyFilter->GetOutputPort());
	}
	stlWriter->Write();
}

IAFILTER_CREATE(iAExtractSurface)

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
	addParameter("STL output filename", iAValueType::FileNameSave, ".stl");
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

void iATriangulation::performWork(QMap<QString, QVariant> const& parameters) {

	auto surfaceFilter = createSurfaceFilter(parameters, input(0)->vtkImage(), progress());
	if (!surfaceFilter)
	{
		LOG(lvlError, "Generated surface filter is null");
		return;
	}

	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	progress()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());

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
	/*auto fillHoles = vtkSmartPointer<vtkFillHolesFilter>::New();
	fillHoles->SetInputConnection(delaunyFilter->GetOutputPort());
	fillHoles->Update();*/
	//auto polydata = fillHoles->GetOutput();

	auto smoothing = createSmoothing(delaunyFilter);

	stlWriter->SetInputData(smoothing->GetOutput());
	stlWriter->Write();

	setPolyOutput(smoothing->GetOutput());
}


IAFILTER_CREATE(iATriangulation);

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
	addParameter("STL output filename", iAValueType::String, "");
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
