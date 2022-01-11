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
#include "iA3DColoredPolyObjectVis.h"

#include "iA3DPolyObjectActor.h"

#include <iALog.h>
#include <iALookupTable.h>

#include <vtkIdTypeArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTable.h>
#include <vtkUnsignedCharArray.h>

namespace
{
	const int TransparentAlpha = 32;
}

iA3DColoredPolyObjectVis::iA3DColoredPolyObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color) :
	iA3DObjectVis(objectTable, columnMapping),
	m_colors(vtkSmartPointer<vtkUnsignedCharArray>::New()),
	m_contextAlpha(DefaultContextOpacity),
	m_selectionAlpha(DefaultSelectionOpacity),
	m_baseColor(color),
	m_selectionColor(SelectedColor),
	m_selectionActive(false)
{
}

void iA3DColoredPolyObjectVis::renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & constClassColor, QStandardItem* /*activeClassItem*/)
{
	m_selection = sortedSelInds;
	QColor BackColor(128, 128, 128, 0);
	size_t currentObjectIndexInSelection = 0;
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
	emit dataChanged();
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
	emit dataChanged();
}

void iA3DColoredPolyObjectVis::multiClassRendering(QList<QColor> const & classColors, QStandardItem* /*rootItem*/, double /*alpha*/)
{
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		int classID = m_objectTable->GetValue(objID, m_objectTable->GetNumberOfColumns() - 1).ToInt();
		setObjectColor(objID, classColors.at(classID));
	}
	emit dataChanged();
}

void iA3DColoredPolyObjectVis::renderOrientationDistribution(vtkImageData* oi)
{
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = getOrientationColor(oi, objID);
		setObjectColor(objID, color);
	}
	emit dataChanged();
}

void iA3DColoredPolyObjectVis::renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* /*extents*/, double /*halfInc*/, int /*filterID*/, double const * /*range*/)
{
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = getLengthColor(ctFun, objID);
		setObjectColor(objID, color);
	}
	emit dataChanged();
}

void iA3DColoredPolyObjectVis::setObjectColor(IndexType objIdx, QColor const & qcolor)
{
	auto const poly = finalPolyData() ? finalPolyData() : polyData();
	auto const colorsAbstr = poly->GetPointData()->GetAbstractArray("Colors");
	if (!colorsAbstr)
	{
		LOG(lvlDebug, "Colors array not found!");
	}
	auto const colors = dynamic_cast<vtkUnsignedCharArray*>(colorsAbstr);
	if (!colors)
	{
		LOG(lvlDebug, "Colors array has wrong type!");
	}
	auto const pntCnt = finalPolyData() ? &iA3DColoredPolyObjectVis::finalObjectPointCount
										: &iA3DColoredPolyObjectVis::objectPointCount;
	auto const startPntIdx = finalPolyData() ? &iA3DColoredPolyObjectVis::finalObjectStartPointIdx
											 : &iA3DColoredPolyObjectVis::objectStartPointIdx;
	unsigned char color[4];
	color[0] = qcolor.red();
	color[1] = qcolor.green();
	color[2] = qcolor.blue();
	color[3] = qcolor.alpha();
	for (int c = 0; c < 4; ++c)
	{
		for (IndexType p = 0; p < (this->*pntCnt)(objIdx); ++p)
		{
			colors->SetComponent( (this->*startPntIdx)(objIdx) + p, c, color[c]);
		}
	}
	colors->Modified();
}

void iA3DColoredPolyObjectVis::setSelectionOpacity(int selectionAlpha)
{
	m_selectionAlpha = selectionAlpha;
}

void iA3DColoredPolyObjectVis::setContextOpacity(int contextAlpha)
{
	m_contextAlpha = contextAlpha;
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
	polyData()->GetPointData()->AddArray(ids);
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
	m_colors->Modified();
	emit dataChanged();
}

double const * iA3DColoredPolyObjectVis::bounds()
{
	return polyData()->GetBounds();
}

void iA3DColoredPolyObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	m_selection = sortedSelInds;
	m_selectionActive = selectionActive;
	updateColorSelectionRendering();
}

std::vector<size_t> const& iA3DColoredPolyObjectVis::selection() const
{
	return m_selection;
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
	m_colors->Modified();
	emit dataChanged();
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

iA3DColoredPolyObjectVis::IndexType iA3DColoredPolyObjectVis::finalObjectPointCount(IndexType ptIdx) const
{
	return objectPointCount(ptIdx);
}

iA3DColoredPolyObjectVis::IndexType iA3DColoredPolyObjectVis::finalObjectStartPointIdx(IndexType ptIdx) const
{
	return objectStartPointIdx(ptIdx);
}

iA3DColoredPolyObjectVis::IndexType iA3DColoredPolyObjectVis::finalAllPointCount() const
{
	IndexType pointCount = 0;
	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		pointCount += finalObjectPointCount(objID);
	}
	return pointCount;
}

/*
vtkAlgorithmOutput* iA3DColoredPolyObjectVis::output()
{
	return nullptr;
}
*/

QSharedPointer<iA3DObjectActor> iA3DColoredPolyObjectVis::createActor(vtkRenderer* ren)
{
	return createPolyActor(ren);
}

QSharedPointer<iA3DPolyObjectActor> iA3DColoredPolyObjectVis::createPolyActor(vtkRenderer* ren)
{
	auto result = QSharedPointer<iA3DPolyObjectActor>::create(ren, this);
	connect(this, &iA3DObjectVis::dataChanged, result.data(), &iA3DPolyObjectActor::updateMapper);
	connect(this, &iA3DObjectVis::renderRequired, result.data(), &iA3DPolyObjectActor::updateRenderer);
	return result;
}
