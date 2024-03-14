// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATrackingGraphItem.h"

#include <vtkContext2D.h>
#include <vtkContextScene.h>
#include <vtkContextMouseEvent.h>
#include <vtkDataSetAttributes.h>
#include <vtkGraph.h>
#include <vtkObjectFactory.h>
#include <vtkTextProperty.h>

// constants
const float			VERTEX_SIZE			= 10;
const float			EDGE_WIDTH			= 2.0f;
const int			MIN_ALPHA			= 100;
const int			DELTA_ALPHA			= 255 - MIN_ALPHA;
const vtkColor4ub	DEFAULT_COLOR (100, 100, 100, 255);

//----------------------------------------------------------------------------
vtkStandardNewMacro(iATrackingGraphItem);


iATrackingGraphItem::iATrackingGraphItem()
{
	focusedVertex = -1;
}

iATrackingGraphItem::~iATrackingGraphItem()
{
}

//----------------------------------------------------------------------------
vtkColor4ub iATrackingGraphItem::VertexColor(vtkIdType vertex)
{
	if (this->GetGraph()->GetVertexData()->GetAbstractArray("ColorR") &&
		this->GetGraph()->GetVertexData()->GetAbstractArray("ColorG") &&
		this->GetGraph()->GetVertexData()->GetAbstractArray("ColorB") &&
		this->GetGraph()->GetVertexData()->GetAbstractArray("Uncertainty"))
	{
		return vtkColor4ub(this->GetGraph()->GetVertexData()->GetAbstractArray("ColorR")->GetVariantValue(vertex).ToInt(),
			this->GetGraph()->GetVertexData()->GetAbstractArray("ColorG")->GetVariantValue(vertex).ToInt(),
			this->GetGraph()->GetVertexData()->GetAbstractArray("ColorB")->GetVariantValue(vertex).ToInt(),
			(1.0 - this->GetGraph()->GetVertexData()->GetAbstractArray("Uncertainty")->GetVariantValue(vertex).ToDouble()) * DELTA_ALPHA + MIN_ALPHA);
	}

	return DEFAULT_COLOR;
}

float iATrackingGraphItem::VertexSize(vtkIdType /*vertex*/)
{
	return VERTEX_SIZE;
}

//----------------------------------------------------------------------------

vtkColor4ub iATrackingGraphItem::EdgeColor(vtkIdType edgeIdx, vtkIdType pointIdx)
{
	//float fraction = static_cast<float>(pointIdx) / (this->NumberOfEdgePoints(edgeIdx) - 1);
	//return vtkColor4ub(fraction * 255, 0, 255 - fraction * 255, 255);
	vtkIdType vert;
	if (pointIdx == 0)
		vert = this->GetGraph()->GetSourceVertex(edgeIdx);
	else
		vert = this->GetGraph()->GetTargetVertex(edgeIdx);

	if (this->GetGraph()->GetVertexData()->GetAbstractArray("ColorR") &&
		this->GetGraph()->GetVertexData()->GetAbstractArray("ColorG") &&
		this->GetGraph()->GetVertexData()->GetAbstractArray("ColorB") &&
		this->GetGraph()->GetVertexData()->GetAbstractArray("Uncertainty"))
	{
		return vtkColor4ub(this->GetGraph()->GetVertexData()->GetAbstractArray("ColorR")->GetVariantValue(vert).ToInt(),
			this->GetGraph()->GetVertexData()->GetAbstractArray("ColorG")->GetVariantValue(vert).ToInt(),
			this->GetGraph()->GetVertexData()->GetAbstractArray("ColorB")->GetVariantValue(vert).ToInt(),
			(1.0 - this->GetGraph()->GetVertexData()->GetAbstractArray("Uncertainty")->GetVariantValue(vert).ToDouble()) * DELTA_ALPHA + MIN_ALPHA);
	}

	return DEFAULT_COLOR;
}

//----------------------------------------------------------------------------
float iATrackingGraphItem::EdgeWidth(vtkIdType vtkNotUsed(lineIdx),
	vtkIdType vtkNotUsed(pointIdx))
{
	return EDGE_WIDTH;
}

//----------------------------------------------------------------------------
bool iATrackingGraphItem::MouseButtonPressEvent(const vtkContextMouseEvent &event)
{
	this->Superclass::MouseButtonPressEvent(event);
	focusedVertex = -1;
	focusedVertex = this->HitVertex(event.GetPos());

	this->GetGraph()->Modified();
	this->GetScene()->SetDirty(true);

	return true;
}

//----------------------------------------------------------------------------
bool iATrackingGraphItem::MouseMoveEvent(const vtkContextMouseEvent &/*event*/)
{
	/*this->Superclass::MouseButtonPressEvent(event);
	focusedVertex = this->HitVertex(event.GetPos());
	this->GetGraph()->GetPoints()->SetPoint(focusedVertex, rand() % 1000 + 20, rand() % 500 + 20, 0);
	this->GetGraph()->Modified();
	this->GetScene()->SetDirty(true);*/

	return true;
}

vtkStdString iATrackingGraphItem::VertexTooltip(vtkIdType vertex)
{
	if (!this->GetGraph()->GetVertexData()->GetAbstractArray("Label"))
	{
		return "N/A";
	}

	return this->GetGraph()->GetVertexData()->GetAbstractArray("Label")->GetVariantValue(vertex).ToString();
}

void iATrackingGraphItem::PaintBuffers(vtkContext2D *painter)
{
	Superclass::PaintBuffers(painter);

	if (focusedVertex >= 0)
	{
		for (int i = 0; i < this->GetGraph()->GetNumberOfVertices(); i++)
		{
			painter->GetTextProp()->SetColor(0, 0, 0);
			painter->GetTextProp()->SetJustificationToCentered();
			painter->GetTextProp()->BoldOff();
			vtkVector2f pos = this->VertexPosition(i);
			vtkStdString label = this->VertexTooltip(i);
			painter->GetTextProp()->SetFontSize(14);
			painter->DrawString(pos.GetX(), pos.GetY() + 5, label);
		}
	}
}
