// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
