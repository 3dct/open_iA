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
#include "iAObserverProgress.h"

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
	//auto vtkProgress = vtkSmartPointer<iAObserverProgress>::New();
	//QObject::connect(vtkProgress.GetPointer(), SIGNAL(oprogress(int)), m_progress, SIGNAL(pprogress(int)));
	//auto stlProgress = vtkSmartPointer<iAObserverProgress>::New();
	//QObject::connect(stlProgress.GetPointer(), SIGNAL(oprogress(int)), m_progress, SIGNAL(pprogress(int)));

	auto surfaceFilter = vtkSmartPointer<vtkMarchingCubes>::New();
	//moSurface->AddObserver(vtkCommand::ProgressEvent, vtkProgress);
	surfaceFilter->SetInputData(m_con->GetVTKImage().GetPointer());
	surfaceFilter->ComputeNormalsOn();
	surfaceFilter->ComputeGradientsOn();
	surfaceFilter->SetValue(0, parameters["Iso value"].toDouble());

	//vtkAlgorithmOutput* output = moSurface->GetOutputPort();

	// these variables need to be created outside of if block, otherwise
	// smart pointers could delete them after if block is finished:
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> sincFilter;
	vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter;

	/*
	// always seems to be of same size as input:
	vtkSmartPointer<vtkQuadricDecimation> simplifyFilter;
	//if (parameters["Quadric decimation"].toBool())
	//{
		simplifyFilter = vtkSmartPointer<vtkQuadricDecimation>::New();
		simplifyFilter->SetInputConnection(surfaceFilter->GetOutputPort());
		simplifyFilter->SetTargetReduction(parameters["Decimation fraction"].toDouble());
		simplifyFilter->Update();
		//output = simplifyFilter->GetOutputPort();
	//}
	*/

	vtkSmartPointer<vtkQuadricClustering> simplifyFilter = vtkSmartPointer<vtkQuadricClustering>::New();
	simplifyFilter->SetNumberOfXDivisions(parameters["Cluster divisions"].toUInt());
	simplifyFilter->SetNumberOfYDivisions(parameters["Cluster divisions"].toUInt());
	simplifyFilter->SetNumberOfZDivisions(parameters["Cluster divisions"].toUInt());
	simplifyFilter->SetInputConnection(surfaceFilter->GetOutputPort());
	/*
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
	//stlWriter->AddObserver(vtkCommand::ProgressEvent, stlProgress);
	stlWriter->SetFileName(parameters["STL output filename"].toString().toStdString().c_str());
	stlWriter->SetInputConnection(simplifyFilter->GetOutputPort());
	stlWriter->Write();
}

IAFILTER_CREATE(iAMarchingCubes)

iAMarchingCubes::iAMarchingCubes() :
	iAFilter("Marching Cubes", "Extract Surface",
		"Extracts a surface along the specified iso value, using the marching cubes algorithm."
		"Subsequently, it optionally simplifies the computed surface using a quadric decimation.<br/>"
		"<br/>"
		"For more information, see the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkMarchingCubes.html\">"
		"Marching Cubes Filter</a> and the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkQuadricDecimation.html\">"
		"Quadric Decimation Filter</a> in the VTK documentation.")
{
	AddParameter("Iso value", Continuous, 1);
	AddParameter("STL output filename", String, "");
	AddParameter("Cluster divisions", Discrete, 1);
	//AddParameter("Quadric decimation", Boolean, true);
	//AddParameter("Decimation fraction", Continuous, 0.01);
	//AddParameter("Smooth windowed sync", Boolean, false);
	//AddParameter("Sinc iterations", Discrete, 1);
	//AddParameter("Smooth poly", Boolean, false);
	//AddParameter("Poly iterations", Discrete, 1);
}


// iAFlyingEdges