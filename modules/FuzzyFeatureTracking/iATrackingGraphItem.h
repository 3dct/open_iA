/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <vtkGraphItem.h>

class iATrackingGraphItem : public vtkGraphItem
{
public:
	static iATrackingGraphItem *New();
	vtkTypeMacro(iATrackingGraphItem, vtkGraphItem);

protected:
	iATrackingGraphItem();
	~iATrackingGraphItem();

	vtkIdType focusedVertex;

	vtkColor4ub VertexColor(vtkIdType vertex) override;
	float VertexSize(vtkIdType vertex) override;
	vtkColor4ub EdgeColor(vtkIdType line, vtkIdType point) override;
	float EdgeWidth(vtkIdType line, vtkIdType point) override;
	bool MouseButtonPressEvent(const vtkContextMouseEvent &event) override;
	bool MouseMoveEvent(const vtkContextMouseEvent &event) override;
	vtkStdString VertexTooltip(vtkIdType vertex) override;
	void PaintBuffers(vtkContext2D *painter) override;
};
