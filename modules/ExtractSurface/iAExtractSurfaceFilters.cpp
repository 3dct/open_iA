/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAConnector.h"
#include "iAProgress.h"

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

void iAMarchingCubes::PerformWork(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkPolyDataAlgorithm> surfaceFilter;

	if (parameters["Algorithm"].toString() == "Marching Cubes")
	{
		auto marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
		marchingCubes->AddObserver(vtkCommand::ProgressEvent, m_progress);
		marchingCubes->SetInputData(m_con->GetVTKImage().GetPointer());
		marchingCubes->ComputeNormalsOn();
		marchingCubes->ComputeGradientsOn();
		marchingCubes->ComputeScalarsOn();
		marchingCubes->SetValue(0, parameters["Iso value"].toDouble());
		surfaceFilter = marchingCubes;
	}
	else
	{
		auto flyingEdges = vtkSmartPointer<vtkFlyingEdges3D>::New();
		flyingEdges->AddObserver(vtkCommand::ProgressEvent, m_progress);
		flyingEdges->SetInputData(m_con->GetVTKImage().GetPointer());
		flyingEdges->SetNumberOfContours(1);
		flyingEdges->SetValue(0, parameters["Iso value"].toDouble());
		flyingEdges->ComputeNormalsOn();
		flyingEdges->ComputeGradientsOn();
		flyingEdges->ComputeScalarsOn();
		flyingEdges->SetArrayComponent(0);
		surfaceFilter = flyingEdges;
	}

	vtkSmartPointer<vtkPolyDataAlgorithm> simplifyFilter;
	if (parameters["Simplification Algorithm"].toString() == "Decimate Pro")
	{
		auto decimatePro = vtkSmartPointer<vtkDecimatePro>::New();
		decimatePro->AddObserver(vtkCommand::ProgressEvent, m_progress);
		decimatePro->SetTargetReduction(parameters["Decimation Target"].toDouble());
		decimatePro->SetPreserveTopology(parameters["Preserve Topology"].toBool());
		decimatePro->SetSplitting(parameters["Splitting"].toBool());
		decimatePro->SetBoundaryVertexDeletion(parameters["Boundary Vertex Deletion"].toBool());
		simplifyFilter = decimatePro;
	}
	else
	{
		auto quadricClustering = vtkSmartPointer<vtkQuadricClustering>::New();
		quadricClustering->AddObserver(vtkCommand::ProgressEvent, m_progress);
		quadricClustering->SetNumberOfXDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetNumberOfYDivisions(parameters["Cluster divisions"].toUInt());
		quadricClustering->SetNumberOfZDivisions(parameters["Cluster divisions"].toUInt());
		simplifyFilter = quadricClustering;
	}
	simplifyFilter->SetInputConnection(surfaceFilter->GetOutputPort());
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
	auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	stlWriter->AddObserver(vtkCommand::ProgressEvent, m_progress);
	stlWriter->SetFileName(parameters["STL output filename"].toString().toStdString().c_str());
	stlWriter->SetInputConnection(simplifyFilter->GetOutputPort());
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
	AddParameter("Extraction Algorithm", Categorical, AlgorithmNames);
	AddParameter("Iso value", Continuous, 1);
	AddParameter("STL output filename", String, "");
	QStringList SimplificationAlgorithms;
	SimplificationAlgorithms << "Quadric Clustering" << "Decimate Pro";
	AddParameter("Simplification Algorithm", Categorical, SimplificationAlgorithms);
	AddParameter("Preserve Topology", Boolean, true);
	AddParameter("Splitting", Boolean, true);
	AddParameter("Boundary Vertex Deletion", Boolean, true);
	AddParameter("Cluster divisions", Discrete, 1);
	AddParameter("Decimation Target", Continuous, 0.9);
	//AddParameter("Smooth windowed sync", Boolean, false);
	//AddParameter("Sinc iterations", Discrete, 1);
	//AddParameter("Smooth poly", Boolean, false);
	//AddParameter("Poly iterations", Discrete, 1);
}
