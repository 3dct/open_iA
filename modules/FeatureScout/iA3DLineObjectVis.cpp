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

#include "mdichild.h"
#include "iARenderer.h"

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRendererCollection.h>
#include <vtkTable.h>
#include <vtkUnsignedCharArray.h>

namespace
{
	const int TransparentAlpha = 32;
	const size_t NoPointIdx = std::numeric_limits<size_t>::max();
}

iA3DLineObjectVis::iA3DLineObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor ):
	iA3DObjectVis(widget, objectTable, columnMapping)
{
	m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_colors->SetNumberOfComponents(4);
	m_colors->SetName("Colors");
	auto pts = vtkSmartPointer<vtkPoints>::New();
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
		pts->InsertNextPoint(first);
		pts->InsertNextPoint(end);
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
	m_linePolyData->SetPoints(pts);
	m_linePolyData->SetLines(lines);
	m_linePolyData->GetPointData()->AddArray(m_colors);
	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_mapper->SetInputData(m_linePolyData);
	m_mapper->SelectColorArray("Colors");
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();
}

void iA3DLineObjectVis::show()
{
	auto actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(m_mapper);
	vtkRenderWindow* renWin = m_widget->GetRenderWindow();
	renWin->GetRenderers()->GetFirstRenderer()->AddActor(actor);
	renWin->GetRenderers()->GetFirstRenderer()->ResetCamera();
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
		classColor.setAlpha(TransparentAlpha);
	}
	else
	{
		classColor.setAlpha(255);
	}
	for ( size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		QColor curColor = (objID == curSelObjID) ?
			SelectedColor :
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
		classColor.setAlpha(TransparentAlpha);
	for ( size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setPolyPointColor(objID, ( labelID > 0 && objID+1 == labelID ) ? SelectedColor : (curClassID == classID) ? classColor : nonClassColor);
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


