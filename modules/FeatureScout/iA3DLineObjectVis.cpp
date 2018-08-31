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
#include "iA3DLineObjectVis.h"

#include "iACsvConfig.h"

#include "iALookupTable.h"
#include "iARenderer.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkIdFilter.h>
#include <vtkLine.h>
#include <vtkOutlineFilter.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRendererCollection.h>
#include <vtkTable.h>
#include <vtkUnsignedCharArray.h>

namespace
{
	const size_t NoPointIdx = std::numeric_limits<size_t>::max();
}

iA3DLineObjectVis::iA3DLineObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor ):
	iA3DObjectVis(widget, objectTable, columnMapping),
	m_contextAlpha(DefaultContextOpacity),
	m_selectionAlpha(DefaultSelectionOpacity),
	m_selectionColor(SelectedColor),
	m_baseColor(128, 128, 128),
	m_selectionActive(false),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New())
{
	m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_colors->SetNumberOfComponents(4);
	m_colors->SetName("Colors");
	m_points = vtkSmartPointer<vtkPoints>::New();
	m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
	auto lines = vtkSmartPointer<vtkCellArray>::New();
	unsigned char c[4];
	c[0] = neutralColor.red();
	c[1] = neutralColor.green();
	c[2] = neutralColor.blue();
	c[3] = neutralColor.alpha();
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		float first[3], end[3];
		for (int i = 0; i < 3; ++i)
		{
			first[i] = m_objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::StartX + i)).ToFloat();
			end[i] = m_objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::EndX + i)).ToFloat();
		}
		m_points->InsertNextPoint(first);
		m_points->InsertNextPoint(end);
		auto line = vtkSmartPointer<vtkLine>::New();
		line->GetPointIds()->SetId(0, 2 * row);     // the index of line start point in pts
		line->GetPointIds()->SetId(1, 2 * row + 1); // the index of line end point in pts
		lines->InsertNextCell(line);
#if (VTK_MAJOR_VERSION < 7) || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION==0)
		m_colors->InsertNextTupleValue(c);
		m_colors->InsertNextTupleValue(c);
#else
		m_colors->InsertNextTypedTuple(c);
		m_colors->InsertNextTypedTuple(c);
#endif
	}
	m_linePolyData->SetPoints(m_points);
	m_linePolyData->SetLines(lines);
	m_linePolyData->GetPointData()->AddArray(m_colors);

	auto ids = vtkSmartPointer<vtkIdTypeArray>::New();
	ids->SetName("OriginalIds");
	vtkIdType numPoints = objectTable->GetNumberOfRows() * 2;
	ids->SetNumberOfTuples(numPoints);
	for (vtkIdType id = 0; id < numPoints; ++id)
		ids->SetTuple1(id, id);
	m_linePolyData->GetPointData()->AddArray(ids);

	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_mapper->SetInputData(m_linePolyData);
	m_mapper->SelectColorArray("Colors");
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();

	m_outlineFilter->SetInputData(m_linePolyData);
	m_outlineMapper->SetInputConnection(m_outlineFilter->GetOutputPort());
	m_outlineActor->GetProperty()->SetColor(0, 0, 0);
	m_outlineActor->PickableOff();
	m_outlineActor->SetMapper(m_outlineMapper);
}

void iA3DLineObjectVis::updateValues( std::vector<std::vector<double> > const & values )
{
	for (int f = 0; f < values.size(); ++f)
	{
		m_points->SetPoint(2 * f, values[f].data());
		m_points->SetPoint(2 * f + 1, values[f].data() + 3);
	}
	m_points->Modified();
	updatePolyMapper();
}

void iA3DLineObjectVis::show()
{
	m_actor = vtkSmartPointer<vtkActor>::New();
	m_actor->SetMapper(m_mapper);
	vtkRenderWindow* renWin = m_widget->GetRenderWindow();
	renWin->GetRenderers()->GetFirstRenderer()->AddActor(m_actor);
}

void iA3DLineObjectVis::hide()
{
	if (!m_actor)
		return;
	vtkRenderWindow* renWin = m_widget->GetRenderWindow();
	renWin->GetRenderers()->GetFirstRenderer()->RemoveActor(m_actor);
}

void iA3DLineObjectVis::renderSelection( std::vector<size_t> const & sortedSelInds, int classID, QColor const & constClassColor, QStandardItem* activeClassItem )
{
	QColor BackColor(128, 128, 128, 0);
	int currentObjectIndexInSelection = 0;
	size_t curSelObjID = NoPointIdx;
	QColor classColor(constClassColor);
	if ( sortedSelInds.size() > 0 )
	{
		curSelObjID = sortedSelInds[currentObjectIndexInSelection];
		classColor.setAlpha(m_contextAlpha);
	}
	for ( size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		QColor curColor = (objID == curSelObjID) ?
			m_selectionColor :
			((curClassID == classID) ?
				classColor :
				BackColor);
		setPolyPointColor(objID, curColor);
		if (objID == curSelObjID)
		{
			++currentObjectIndexInSelection;
			if (currentObjectIndexInSelection < sortedSelInds.size())
				curSelObjID = sortedSelInds[currentObjectIndexInSelection];
		}
	}
	updatePolyMapper();
}

void iA3DLineObjectVis::renderSingle( int labelID, int classID, QColor const & constClassColor, QStandardItem* activeClassItem )
{
	QColor classColor(constClassColor);
	QColor nonClassColor = QColor(0, 0, 0, 0);
	if ( labelID > 0)
		classColor.setAlpha(m_contextAlpha);
	for ( size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setPolyPointColor(objID, ( labelID > 0 && objID+1 == labelID ) ? m_selectionColor : (curClassID == classID) ? classColor : nonClassColor);
	}
	updatePolyMapper();
}

void iA3DLineObjectVis::multiClassRendering( QList<QColor> const & classColors, QStandardItem* rootItem, double alpha )
{
	for ( size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		int classID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setPolyPointColor(objID, classColors.at(classID));
	}
	updatePolyMapper();
}

void iA3DLineObjectVis::renderOrientationDistribution ( vtkImageData* oi )
{
	for ( size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		QColor color = getOrientationColor( oi, objID );
		setPolyPointColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DLineObjectVis::renderLengthDistribution( vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range )
{
	for ( int objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		QColor color = getLengthColor( ctFun, objID );
		setPolyPointColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DLineObjectVis::setPolyPointColor(int ptIdx, QColor const & qcolor)
{
	unsigned char color[4];
	color[0] = qcolor.red();
	color[1] = qcolor.green();
	color[2] = qcolor.blue();
	color[3] = qcolor.alpha();
	for (int c = 0; c < 4; ++c)
	{
		m_colors->SetComponent(ptIdx * 2, c, color[c]);
		m_colors->SetComponent(ptIdx * 2 + 1, c, color[c]);
	}
}

void iA3DLineObjectVis::updatePolyMapper()
{
	m_colors->Modified();
	m_mapper->Update();
	updateRenderer();
}

vtkPolyData* iA3DLineObjectVis::getLinePolyData()
{
	return m_linePolyData;
}

void iA3DLineObjectVis::setSelectionOpacity(int selectionAlpha)
{
	m_selectionAlpha = selectionAlpha;
}


void iA3DLineObjectVis::setContextOpacity(int contextAlpha)
{
	m_contextAlpha = contextAlpha;
}

void iA3DLineObjectVis::setColor(QColor const &color)
{
	m_baseColor = color;
	m_colorParamIdx = -1;
	m_lut.clear();
	updateColorSelectionRendering();
}

void iA3DLineObjectVis::setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIndex)
{
	m_lut = lut;
	m_colorParamIdx = paramIndex;
	updateColorSelectionRendering();
}

void iA3DLineObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	m_selection = sortedSelInds;
	m_selectionActive = selectionActive;
	updateColorSelectionRendering();
}

void iA3DLineObjectVis::updateColorSelectionRendering()
{
	size_t curSelIdx = 0;
	for (size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = m_baseColor;
		if (m_lut)
		{
			double curValue = m_objectTable->GetValue(objID, m_colorParamIdx).ToDouble();
			color = m_lut->getQColor(curValue);
		}
		if (m_selectionActive)
		{
			if (curSelIdx < m_selection.size() && objID == m_selection[curSelIdx])
			{
				color.setAlpha(m_selectionAlpha);
				++curSelIdx;
			}
			else
				color.setAlpha(m_contextAlpha);
		}
		else
			color.setAlpha(m_selectionAlpha);
		setPolyPointColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DLineObjectVis::showBoundingBox()
{
	m_outlineMapper->Update();
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_outlineActor);
	updateRenderer();
}

void iA3DLineObjectVis::hideBoundingBox()
{
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_outlineActor);
	updateRenderer();
}

iA3DLineObjectVis::~iA3DLineObjectVis()
{
	hide();
	hideBoundingBox();
}
