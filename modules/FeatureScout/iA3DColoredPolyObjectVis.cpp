/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
}

iA3DColoredPolyObjectVis::iA3DColoredPolyObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color) :
	iA3DObjectVis(ren, objectTable, columnMapping),
	m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_colors(vtkSmartPointer<vtkUnsignedCharArray>::New()),
	m_actor(vtkSmartPointer<vtkActor>::New()),
	m_visible(false),
	m_contextAlpha(DefaultContextOpacity),
	m_selectionAlpha(DefaultSelectionOpacity),
	m_baseColor(color),
	m_selectionColor(SelectedColor),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New()),
	m_clippingPlanesEnabled(false),
	m_selectionActive(false)
{
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();
	m_actor->SetMapper(m_mapper);
}

void iA3DColoredPolyObjectVis::show()
{
	if (m_visible)
	{
		return;
	}
	m_ren->AddActor(m_actor);
	m_visible = true;
}

void iA3DColoredPolyObjectVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_actor);
	m_visible = false;
}

void iA3DColoredPolyObjectVis::updateRenderer()
{
	if (m_visible)
	{
		iA3DObjectVis::updateRenderer();
	}
}

void iA3DColoredPolyObjectVis::renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & constClassColor, QStandardItem* /*activeClassItem*/)
{
	QColor BackColor(128, 128, 128, 0);
	int currentObjectIndexInSelection = 0;
	IndexType curSelObjID = -1;
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
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		QColor curColor = (objID == curSelObjID) ?
			SelectedColor :
			((curClassID == classID) ?
				classColor :
				BackColor);
		setObjectColor(objID, curColor);
		if (objID == curSelObjID)
		{
			++currentObjectIndexInSelection;
			if (currentObjectIndexInSelection < sortedSelInds.size())
			{
				curSelObjID = sortedSelInds[currentObjectIndexInSelection];
			}
		}
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::renderSingle(IndexType selectedObjID, int classID, QColor const & constClassColor, QStandardItem* /*activeClassItem*/)
{
	QColor classColor(constClassColor);
	QColor nonClassColor = QColor(0, 0, 0, 0);
	if (selectedObjID > 0)
	{
		classColor.setAlpha(TransparentAlpha);
	}
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		int curClassID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setObjectColor(objID, (selectedObjID > 0 && objID + 1 == selectedObjID) ? SelectedColor : (curClassID == classID) ? classColor : nonClassColor);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::multiClassRendering(QList<QColor> const & classColors, QStandardItem* /*rootItem*/, double /*alpha*/)
{
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		int classID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setObjectColor(objID, classColors.at(classID));
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::renderOrientationDistribution(vtkImageData* oi)
{
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = getOrientationColor(oi, objID);
		setObjectColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* /*extents*/, double /*halfInc*/, int /*filterID*/, double const * /*range*/)
{
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = getLengthColor(ctFun, objID);
		setObjectColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::setObjectColor(IndexType objIdx, QColor const & qcolor)
{
	unsigned char color[4];
	color[0] = qcolor.red();
	color[1] = qcolor.green();
	color[2] = qcolor.blue();
	color[3] = qcolor.alpha();
	for (int c = 0; c < 4; ++c)
	{
		for (IndexType p = 0; p < objectPointCount(objIdx); ++p)
		{
			m_colors->SetComponent(objectStartPointIdx(objIdx) + p, c, color[c]);
		}
	}
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

void iA3DColoredPolyObjectVis::setShowWireFrame(bool show)
{
	if (show)
	{
		m_actor->GetProperty()->SetRepresentationToWireframe();
	}
	else
	{
		m_actor->GetProperty()->SetRepresentationToSurface();
	}
	updatePolyMapper();
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
	ids->SetNumberOfTuples(allPointCount());
	for (vtkIdType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		for (vtkIdType pt = 0; pt < objectPointCount(objID); ++pt)
		{
			ids->SetTuple1(objectStartPointIdx(objID) + pt, objID);
		}
	}
	getPolyData()->GetPointData()->AddArray(ids);
}

void iA3DColoredPolyObjectVis::setupColors()
{
	m_colors->SetNumberOfComponents(4);
	m_colors->SetName("Colors");
	unsigned char c[4];
	c[0] = m_baseColor.red();
	c[1] = m_baseColor.green();
	c[2] = m_baseColor.blue();
	c[3] = m_baseColor.alpha();
	size_t colorCount = allPointCount();
	for (size_t ptIdx = 0; ptIdx < colorCount; ++ptIdx)
	{
		m_colors->InsertNextTypedTuple(c);
	}
	m_mapper->SelectColorArray("Colors");
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
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = m_baseColor;
		if (m_lut)
		{
			double curValue = m_objectTable->GetValue(objID, m_colorParamIdx).ToDouble();
			color = m_lut->getQColor(curValue);
		}
		if (m_selectionActive)
		{
			if (curSelIdx < m_selection.size() && static_cast<size_t>(objID) == m_selection[curSelIdx])
			{
				color.setAlpha(m_selectionAlpha);
				++curSelIdx;
			}
			else
			{
				color.setAlpha(m_contextAlpha);
			}
		}
		else
		{
			color.setAlpha(m_selectionAlpha);
		}
		setObjectColor(objID, color);
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::setClippingPlanes(vtkPlane* planes[3])
{
	if (m_clippingPlanesEnabled)
	{
		return;
	}
	m_clippingPlanesEnabled = true;
	for (int i = 0; i < 3; ++i)
	{
		m_mapper->AddClippingPlane(planes[i]);
	}
}

void iA3DColoredPolyObjectVis::removeClippingPlanes()
{
	m_mapper->RemoveAllClippingPlanes();
	m_clippingPlanesEnabled = false;
}

iA3DColoredPolyObjectVis::IndexType iA3DColoredPolyObjectVis::objectPointCount(IndexType /*ptIdx*/) const
{
	return DefaultPointsPerObject;
}

iA3DColoredPolyObjectVis::IndexType iA3DColoredPolyObjectVis::objectStartPointIdx(IndexType ptIdx) const
{
	return ptIdx * DefaultPointsPerObject;
}

iA3DColoredPolyObjectVis::IndexType iA3DColoredPolyObjectVis::allPointCount() const
{
	IndexType pointCount = 0;
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		pointCount += objectPointCount(objID);
	}
	return pointCount;
}