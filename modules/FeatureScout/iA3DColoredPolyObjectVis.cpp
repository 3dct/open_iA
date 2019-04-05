/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iA3DColoredPolyObjectVis.h"

#include <iALookupTable.h>
#include <iAVtkWidget.h>

#include <vtkActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkOutlineFilter.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkTable.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVersion.h>

namespace
{
	const int TransparentAlpha = 32;
	const size_t NoPointIdx = std::numeric_limits<size_t>::max();
}

iA3DColoredPolyObjectVis::iA3DColoredPolyObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, size_t pointsPerObject) :
	iA3DObjectVis(ren, objectTable, columnMapping),
	m_colors(vtkSmartPointer<vtkUnsignedCharArray>::New()),
	m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_pointsPerObject(pointsPerObject),
	m_contextAlpha(DefaultContextOpacity),
	m_selectionAlpha(DefaultSelectionOpacity),
	m_selectionColor(SelectedColor),
	m_baseColor(128, 128, 128),
	m_actor(vtkSmartPointer<vtkActor>::New()),
	m_visible(false),
	m_selectionActive(false),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New())
{
	m_colors->SetNumberOfComponents(4);
	m_colors->SetName("Colors");
	unsigned char c[4];
	c[0] = color.red();
	c[1] = color.green();
	c[2] = color.blue();
	c[3] = color.alpha();
	size_t colorCount = m_objectTable->GetNumberOfRows()*pointsPerObject;
	for (size_t row = 0; row < colorCount; ++row)
	{
#if (VTK_MAJOR_VERSION < 7) || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION==0)
		m_colors->InsertNextTupleValue(c);
#else
		m_colors->InsertNextTypedTuple(c);
#endif
	}
	m_mapper->SelectColorArray("Colors");
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();
	m_actor->SetMapper(m_mapper);
}

void iA3DColoredPolyObjectVis::show()
{
	if (m_visible)
		return;
	m_ren->AddActor(m_actor);
	m_ren->ResetCamera();
	m_visible = true;
}

void iA3DColoredPolyObjectVis::hide()
{
	if (!m_visible)
		return;
	m_ren->RemoveActor(m_actor);
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

vtkSmartPointer<vtkActor> iA3DColoredPolyObjectVis::getActor()
{
	return m_actor;
}

void iA3DColoredPolyObjectVis::setupBoundingBox()
{
	m_outlineFilter->SetInputData(getPolyData());
	m_outlineMapper->SetInputConnection(m_outlineFilter->GetOutputPort());
	m_outlineActor->GetProperty()->SetColor(0, 0, 0);
	m_outlineActor->PickableOff();
	m_outlineActor->SetMapper(m_outlineMapper);
}


void iA3DColoredPolyObjectVis::setupOriginalIds()
{
	auto ids = vtkSmartPointer<vtkIdTypeArray>::New();
	ids->SetName("OriginalIds");
	vtkIdType numPoints = m_objectTable->GetNumberOfRows() * m_pointsPerObject;
	ids->SetNumberOfTuples(numPoints);
	for (vtkIdType id = 0; id < m_objectTable->GetNumberOfRows(); ++id)
		for (vtkIdType objectPoints = 0; objectPoints < m_pointsPerObject; ++objectPoints)
		ids->SetTuple1(id*m_pointsPerObject + objectPoints, id);
	getPolyData()->GetPointData()->AddArray(ids);
}

void iA3DColoredPolyObjectVis::showBoundingBox()
{
	m_outlineMapper->Update();
	m_ren->AddActor(m_outlineActor);
	updateRenderer();
}

void iA3DColoredPolyObjectVis::hideBoundingBox()
{
	m_ren->RemoveActor(m_outlineActor);
	updateRenderer();
}

double const * iA3DColoredPolyObjectVis::bounds()
{
	return getPolyData()->GetBounds();
}

void iA3DColoredPolyObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	m_selection = sortedSelInds;
	m_selectionActive = selectionActive;
	updateColorSelectionRendering();
}

void iA3DColoredPolyObjectVis::setColor(QColor const &color)
{
	m_baseColor = color;
	m_colorParamIdx = -1;
	m_lut.clear();
	updateColorSelectionRendering();
}

void iA3DColoredPolyObjectVis::setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIndex)
{
	m_lut = lut;
	m_colorParamIdx = paramIndex;
	updateColorSelectionRendering();
}

void iA3DColoredPolyObjectVis::updateColorSelectionRendering()
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

