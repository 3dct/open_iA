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
#include <vtkSTLWriter.h>
#include <vtkSmartPointer.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkSmoothPolyDataFilter.h>

void iAMarchingCubes::PerformWork(QMap<QString, QVariant> const & parameters)
{
	//auto vtkProgress = vtkSmartPointer<iAObserverProgress>::New();
	//QObject::connect(vtkProgress.GetPointer(), SIGNAL(oprogress(int)), m_progress, SIGNAL(pprogress(int)));
	//auto stlProgress = vtkSmartPointer<iAObserverProgress>::New();
	//QObject::connect(stlProgress.GetPointer(), SIGNAL(oprogress(int)), m_progress, SIGNAL(pprogress(int)));

	vtkSmartPointer<vtkMarchingCubes> moSurface = vtkSmartPointer<vtkMarchingCubes>::New();
	//moSurface->AddObserver(vtkCommand::ProgressEvent, vtkProgress);
	moSurface->SetInputData(m_con->GetVTKImage().GetPointer());
	moSurface->ComputeNormalsOn();
	moSurface->ComputeGradientsOn();
	moSurface->SetValue(0, parameters["Iso value"].toDouble());

	vtkSmartPointer<vtkWindowedSincPolyDataFilter>

	vtkSmartPointer<vtkSTLWriter> stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	//stlWriter->AddObserver(vtkCommand::ProgressEvent, stlProgress);
	stlWriter->SetFileName(parameters["STL output filename"].toString().toStdString().c_str());
	stlWriter->SetInputConnection(moSurface->GetOutputPort());
	stlWriter->Write();
}

IAFILTER_CREATE(iAMarchingCubes)

iAMarchingCubes::iAMarchingCubes() :
	iAFilter("Marching Cubes", "Extract Surface",
		"Extracts a surface along the specified iso value, using the marching cubes algorithm.<br/>"
		"<br/>"
		"For more information, see the "
		"<a href=\"https://www.vtk.org/doc/nightly/html/classvtkMarchingCubes.html\">"
		"Marching Cubes Filter</a> in the VTK documentation.")
{
	AddParameter("Iso value", Continuous, 1);
	AddParameter("STL output filename", String, "");
	AddParameter("Smooth windowed sync", Boolean, true);
	AddParameter("Smooth poly", Boolean, false);
}


// iAFlyingEdges