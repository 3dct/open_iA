/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include "iAConsole.h"

#include <vtkActor.h>
#include <vtkAreaPicker.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractGeometry.h>
#include <vtkIdTypeArray.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPlane.h>
#include <vtkPlanes.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVertexGlyphFilter.h>

#include <QObject>

#include <set>
#include <vector>

class InteractorStyle : public QObject, public vtkInteractorStyleRubberBandPick
{
	Q_OBJECT
public:
	static InteractorStyle* New();
	vtkTypeMacro(InteractorStyle, vtkInteractorStyleRubberBandPick);

	InteractorStyle() {}

	virtual void OnLeftButtonUp()
	{
		// Forward events
		vtkInteractorStyleRubberBandPick::OnLeftButtonUp();

		if (!Points)
			return;

		vtkPlanes* frustum = static_cast<vtkAreaPicker*>(this->GetInteractor()->GetPicker())->GetFrustum();

		vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
		extractGeometry->SetImplicitFunction(frustum);
		extractGeometry->SetInputData(this->Points);
		extractGeometry->Update();

		vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
		glyphFilter->SetInputConnection(extractGeometry->GetOutputPort());
		glyphFilter->Update();

		vtkPolyData* selected = glyphFilter->GetOutput();
		DEBUG_LOG(QString("Selected %1 points and %2 cells.")
			.arg(selected->GetNumberOfPoints())
			.arg(selected->GetNumberOfCells()));

		vtkPointData* pointData = selected->GetPointData();
		vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(pointData->GetArray("OriginalIds"));
		if (!ids)
			return;

		std::set<size_t> selset;
		for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
		{
			selset.insert(ids->GetValue(i) / 2);
			//DEBUG_LOG(QString("    %1: %2").arg(i).arg(ids->GetValue(i)));
		}
		std::vector<size_t> selection;
		std::copy(selset.begin(), selset.end(), std::back_inserter(selection));
		emit selectionChanged(selection);
	}

	void SetInput(vtkSmartPointer<vtkPolyData> points) { this->Points = points; }
signals:
	void selectionChanged(std::vector<size_t> const &);
private:
	vtkSmartPointer<vtkPolyData> Points;
};
