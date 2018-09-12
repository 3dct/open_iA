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

#include "iA3DColoredPolyObjectVis.h"

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkTable.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include "QVTKOpenGLWidget.h"
#else
#include "QVTKWidget2.h"
#endif

namespace
{
	const int TransparentAlpha = 32;
	const size_t NoPointIdx = std::numeric_limits<size_t>::max();
}

iA3DColoredPolyObjectVis::iA3DColoredPolyObjectVis(iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, size_t pointsPerObject) :
	iA3DObjectVis(widget, objectTable, columnMapping),
	m_colors(vtkSmartPointer<vtkUnsignedCharArray>::New()),
	m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_pointsPerObject(pointsPerObject),
	m_contextAlpha(DefaultContextOpacity),
	m_selectionAlpha(DefaultSelectionOpacity),
	m_selectionColor(SelectedColor),
	m_baseColor(128, 128, 128),
	m_actor(vtkSmartPointer<vtkActor>::New()),
	m_visible(false)
{
	m_colors->SetNumberOfComponents(4);
	m_colors->SetName("Colors");
	unsigned char c[4];
	c[0] = color.red();
	c[1] = color.green();
	c[2] = color.blue();
	c[3] = color.alpha();
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		for (size_t point = 0; point < pointsPerObject; ++point)
		{
#if (VTK_MAJOR_VERSION < 7) || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION==0)
			m_colors->InsertNextTupleValue(c);
#else
			m_colors->InsertNextTypedTuple(c);
#endif
		}
	}
	m_mapper->SelectColorArray("Colors");
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();
	m_actor->SetMapper(m_mapper);
}

void iA3DColoredPolyObjectVis::show()
{
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_actor);
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCamera();
	m_visible = true;
}

void iA3DColoredPolyObjectVis::hide()
{
	// TODO: problematic if main window is being shut down!
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_actor);
	m_visible = false;
}

void iA3DColoredPolyObjectVis::renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & constClassColor, QStandardItem* activeClassItem)
{
	QColor BackColor(128, 128, 128, 0);
	int currentObjectIndexInSelection = 0;
	size_t curSelObjID = NoPointIdx;
	QColor classColor(constClassColor);
	if (sortedSelInds.size() > 0)
	{
		curSelObjID = sortedSelInds[currentObjectIndexInSelection];
		classColor.setAlpha(TransparentAlpha);
	}
	else
	{
		classColor.setAlpha(255);
	}
	for (size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
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

void iA3DColoredPolyObjectVis::renderSingle(int labelID, int classID, QColor const & constClassColor, QStandardItem* activeClassItem)
{
	QColor classColor(constClassColor);
	QColor nonClassColor = QColor(0, 0, 0, 0);
	if (labelID > 0)
		classColor.setAlpha(TransparentAlpha);
	for (size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setPolyPointColor(objID, (labelID > 0 && objID + 1 == labelID) ? SelectedColor : (curClassID == classID) ? classColor : nonClassColor);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::multiClassRendering(QList<QColor> const & classColors, QStandardItem* rootItem, double alpha)
{
	for (size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		int classID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setPolyPointColor(objID, classColors.at(classID));
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::renderOrientationDistribution(vtkImageData* oi)
{
	for (size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = getOrientationColor(oi, objID);
		setPolyPointColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range)
{
	for (int objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = getLengthColor(ctFun, objID);
		setPolyPointColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::setPolyPointColor(int ptIdx, QColor const & qcolor)
{
	unsigned char color[4];
	color[0] = qcolor.red();
	color[1] = qcolor.green();
	color[2] = qcolor.blue();
	color[3] = qcolor.alpha();
	for (int c = 0; c < 4; ++c)
		for (size_t p=0; p<m_pointsPerObject; ++p)
			m_colors->SetComponent(ptIdx * m_pointsPerObject + p, c, color[c]);
}

void iA3DColoredPolyObjectVis::updatePolyMapper()
{
	m_colors->Modified();
	m_mapper->Update();
	updateRenderer();
}

bool iA3DColoredPolyObjectVis::visible() const
{
	return m_visible;
}

void iA3DColoredPolyObjectVis::setSelectionOpacity(int selectionAlpha)
{
	m_selectionAlpha = selectionAlpha;
}

void iA3DColoredPolyObjectVis::setContextOpacity(int contextAlpha)
{
	m_contextAlpha = contextAlpha;
}
