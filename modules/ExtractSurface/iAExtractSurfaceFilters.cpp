/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "io/iAFileUtils.h"

#include <iAConnector.h>
#include <iAProgress.h>
#include <vtkCleanPolyData.h>
#include <vtkDecimatePro.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkQuadricDecimation.h>
#include <vtkQuadricClustering.h>
#include <vtkSmartPointer.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkSTLWriter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkFillHolesFilter.h>
#include "TriangulationFilter.h"
#include "iAConsole.h"

void iAMarchingCubes::performWork(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter = vtkSmartPointer<vtkPolyDataAlgorithm>::New();
	TriangulationFilter surfaceGenFilter; 



	surfaceFilter = surfaceGenFilter.surfaceFilterParametrisation
	(parameters, input()[0]->vtkImage(), progress()); 
	
	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	progress()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());

	if (!surfaceFilter) { 
		DEBUG_LOG("Generated surface filter is null");
		return; 
	}


	progress()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());


	/*if (parameters["Algorithm"].toString() == "Marching Cubes")
	{
		auto marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
		progress()->observe(marchingCubes);
		marchingCubes->SetInputData(input()[0]->vtkImage().GetPointer());
		marchingCubes->ComputeNormalsOn();
		marchingCubes->ComputeGradientsOn();
		marchingCubes->ComputeScalarsOn();
		marchingCubes->SetValue(0, parameters["Iso value"].toDouble());
		surfaceFilter = marchingCubes;
	}
	else
	{
		auto flyingEdges = vtkSmartPointer<vtkFlyingEdges3D>::New();
		progress()->observe(flyingEdges);
		flyingEdges->SetInputData(input()[0]->vtkImage().GetPointer());
		flyingEdges->SetNumberOfContours(1);
		flyingEdges->SetValue(0, parameters["Iso value"].toDouble());
		flyingEdges->ComputeNormalsOn();
		flyingEdges->ComputeGradientsOn();
		flyingEdges->ComputeScalarsOn();
		flyingEdges->SetArrayComponent(0);
		surfaceFilter = flyingEdges;
	}
*/
	vtkSmartPointer<vtkPolyDataAlgorithm> simplifyFilter = vtkSmartPointer<vtkPolyDataAlgorithm>::New();

	//if ()

	if (parameters["Simplification Algorithm"].toString() == "None")
	{
		stlWriter->SetInputConnection(surfaceFilter->GetOutputPort());
	}
	else {

		simplifyFilter = surfaceGenFilter.pointsDecimation(parameters, surfaceFilter, progress());
		stlWriter->SetInputConnection(simplifyFilter->GetOutputPort());
	}
	/*if (parameters["Simplification Algorithm"].toString() == "Decimate Pro")
	{
		auto decimatePro = vtkSmartPointer<vtkDecimatePro>::New();
		progress()->observe(decimatePro);
		decimatePro->SetTargetReduction(parameters["Decimation Target"].toDouble());
		decimatePro->SetPreserveTopology(parameters["Preserve Topology"].toBool());
		decimatePro->SetSplitting(parameters["Splitting"].toBool());
		decimatePro->SetBoundaryVertexDeletion(parameters["Boundary Vertex Deletion"].toBool());
		decimatePro->SetInputConnection(surfaceFilter->GetOutputPort());
		simplifyFilter = decimatePro;
	}
	else if (parameters["Simplification Algorithm"].toString() == "Quadric Clustering")
	{
		auto quadricClustering = vtkSmartPointer<vtkQuadricClustering>::New();
		progress()->observe(quadricClustering);
		quadricClustering->SetNumberOfXDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetNumberOfYDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetNumberOfZDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetInputConnection(surfaceFilter->GetOutputPort());
		simplifyFilter = quadricClustering;
	}*/

	/*
	// smoothing?
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> sincFilter;
	vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter;
	if (parameters["Smooth windowed sinc"].toBool())
	{
		sincFilter = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
		sincFilter->SetInputConnection(output);
		sincFilter->SetNumberOfIterations(parameters["Sinc iterations"].toUInt());
		output = sincFilter->GetOutputPort();
	}
	if (parameters["Smooth poly"].toBool())
	{
		smoothFilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
		smoothFilter->SetInputConnection(output);
		smoothFilter->SetNumberOfIterations(parameters["Poly iterations"].toUInt());
		output = sincFilter->GetOutputPort();
	}
	*/
	/*auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	progress()->observe(stlWriter);
	stlWriter->SetFileName( getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());
	if (parameters["Simplification Algorithm"].toString() == "None")
	{
		stlWriter->SetInputConnection(surfaceFilter->GetOutputPort());
	}
	else
	{
		stlWriter->SetInputConnection(simplifyFilter->GetOutputPort());
	}*/
	stlWriter->Write();
}

IAFILTER_CREATE(iAMarchingCubes)

iAMarchingCubes::iAMarchingCubes() :
	iAFilter("Extract Surface", "Extract Surface",
		"Extracts a surface along the specified iso value.<br/>"
		"A surface is extracted at the given Iso value, either using marching cubes or "
		"flying edges algorithm. The mesh is subsequently simplified using either "
		"a quadric clustering or the DecimatePro algorithm.<br/>"
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
	addParameter("Extraction Algorithm", Categorical, AlgorithmNames);
	addParameter("Iso value", Continuous, 1);
	addParameter("STL output filename", String, "");
	QStringList SimplificationAlgorithms;
	SimplificationAlgorithms << "Quadric Clustering" << "Decimate Pro" << "None";
	addParameter("Simplification Algorithm", Categorical, SimplificationAlgorithms);
	addParameter("Preserve Topology", Boolean, true);
	addParameter("Splitting", Boolean, true);
	addParameter("Boundary Vertex Deletion", Boolean, true);
	addParameter("Decimation Target", Continuous, 0.9);
	addParameter("Cluster divisions", Discrete, 128);
	//addParameter("Smooth windowed sync", Boolean, false);
	//addParameter("Sinc iterations", Discrete, 1);
	//addParameter("Smooth poly", Boolean, false);
	//addParameter("Poly iterations", Discrete, 1);
}

void iATriangulation::performWork(QMap<QString, QVariant> const& parameters) {

	vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter = vtkSmartPointer<vtkPolyDataAlgorithm>::New();
	TriangulationFilter surfaceGenFilter;



	surfaceFilter = surfaceGenFilter.surfaceFilterParametrisation
	(parameters, input()[0]->vtkImage(), progress());

	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();

	progress()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());

	if (!surfaceFilter) {
		DEBUG_LOG("Generated surface filter is null");
		return;
	}


	/*progress()->observe(stlWriter);
	stlWriter->SetFileName(getLocalEncodingFileName(parameters["STL output filename"].toString()).c_str());*/
	

	vtkSmartPointer<vtkPolyDataAlgorithm> simplifyFilter = vtkSmartPointer<vtkPolyDataAlgorithm>::New();
	vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
	//if ()

	if (parameters["Simplification Algorithm"].toString() == "None")
	{
		cleaner->SetInputConnection(surfaceFilter->GetOutputPort());
		//stlWriter->SetInputConnection(surfaceFilter->GetOutputPort());
	}
	else {

		simplifyFilter = surfaceGenFilter.pointsDecimation(parameters, surfaceFilter, progress());
		//stlWriter->SetInputConnection(simplifyFilter->GetOutputPort());
		cleaner->SetInputConnection(simplifyFilter->GetOutputPort());

	}

	auto cleanTol = parameters["CleanTolerance"].toDouble();
	cleaner->SetTolerance(cleanTol);
	//clean duplicated points
	cleaner->Update(); 
	DEBUG_LOG("perfom delauy3d"); 
	double alpha = parameters["Alpha"].toDouble();
	double offset = parameters["Offset"].toDouble(); 
	double tolerance = parameters["Tolerance"].toDouble();

	DEBUG_LOG(QString("%1").arg(alpha)); 

	auto delaunyFilter = surfaceGenFilter.performDelaunay(parameters, cleaner, alpha,offset,tolerance, progress());
	/*auto fillHoles = vtkSmartPointer<vtkFillHolesFilter>::New();
	fillHoles->SetInputConnection(delaunyFilter->GetOutputPort());
	fillHoles->Update();*/
	//auto polydata = fillHoles->GetOutput();

	//if (!polydata) {
	//	"Debug log returned polydata is null"; 
	//	return;
	//}

	auto smoothing = surfaceGenFilter.Smoothing(delaunyFilter);

	stlWriter->SetInputData(smoothing->GetOutput());
	stlWriter->Write(); 
	//stlWriter->SetInputConnection(surfaceFilter->GetOutputPort());
	//	TriangulationFilter.performDelaunay()
	
	
}


IAFILTER_CREATE(iATriangulation);

iATriangulation::iATriangulation() :
	iAFilter("Surface triangulation", "Extract Surface",
		"Extracts a surface along the specified iso value.<br/>"
		"A surface is extracted at the given Iso value, either using marching cubes or "
		"flying edges algorithm. The mesh is subsequently simplified using either "
		"a quadric clustering or the DecimatePro algorithm.<br/>"
		"Based on the thing the surface will be triangulated by means of the Delaunay3D-Algorithm"
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
	addParameter("Extraction Algorithm", Categorical, AlgorithmNames);
	addParameter("Iso value", Continuous, 1);
	addParameter("STL output filename", String, "");
	QStringList SimplificationAlgorithms;
	SimplificationAlgorithms << "Quadric Clustering" << "Decimate Pro" << "None";
	addParameter("Simplification Algorithm", Categorical, SimplificationAlgorithms);
	addParameter("Preserve Topology", Boolean, true);
	addParameter("Splitting", Boolean, true);
	addParameter("Boundary Vertex Deletion", Boolean, true);
	addParameter("Decimation Target", Continuous, 0.9);
	addParameter("Cluster divisions", Discrete, 128);
	addParameter("Alpha", Continuous, 0); 
	addParameter("Offset", Continuous, 0); 
	addParameter("Tolerance", Continuous, 0.001);
	addParameter("CleanTolerance", Continuous, 0);
	//addParameter("Smooth windowed sync", Boolean, false);
	//addParameter("Sinc iterations", Discrete, 1);
	//addParameter("Smooth poly", Boolean, false);
	//addParameter("Poly iterations", Discrete, 1);
}
