// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAColoredPolyObjectVis.h"

#include "iAObjectsData.h"
#include "iAPolyObjectVisActor.h"

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

iAColoredPolyObjectVis::iAColoredPolyObjectVis(iAObjectsData const* data, QColor const & color) :
	iAObjectVis(data),
	m_colors(vtkSmartPointer<vtkUnsignedCharArray>::New()),
	m_contextAlpha(DefaultContextOpacity),
	m_selectionAlpha(DefaultSelectionOpacity),
	m_baseColor(color),
	m_selectionColor(SelectedColor),
	m_selectionActive(false)
{
}

void iAColoredPolyObjectVis::renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & constClassColor, QStandardItem* /*activeClassItem*/)
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
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		int curClassID = m_data->m_table->GetValue(objID, m_data->m_table->GetNumberOfColumns() - 1).ToInt();
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

void iAColoredPolyObjectVis::renderSingle(IndexType selectedObjID, int classID, QColor const & constClassColor, QStandardItem* /*activeClassItem*/)
{
	QColor classColor(constClassColor);
	QColor nonClassColor = QColor(0, 0, 0, 0);
	if (selectedObjID > 0)
	{
		classColor.setAlpha(TransparentAlpha);
	}
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		int curClassID = m_data->m_table->GetValue(objID, m_data->m_table->GetNumberOfColumns() - 1).ToInt();
		setObjectColor(objID, (selectedObjID > 0 && objID + 1 == selectedObjID) ? SelectedColor : (curClassID == classID) ? classColor : nonClassColor);
	}
	emit dataChanged();
}

void iAColoredPolyObjectVis::multiClassRendering(QList<QColor> const & classColors, QStandardItem* /*rootItem*/, double /*alpha*/)
{
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		int classID = m_data->m_table->GetValue(objID, m_data->m_table->GetNumberOfColumns() - 1).ToInt();
		setObjectColor(objID, classColors.at(classID));
	}
	emit dataChanged();
}

void iAColoredPolyObjectVis::renderOrientationDistribution(vtkImageData* oi)
{
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		QColor color = getOrientationColor(oi, objID);
		setObjectColor(objID, color);
	}
	emit dataChanged();
}

void iAColoredPolyObjectVis::renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* /*extents*/, double /*halfInc*/, int /*filterID*/, double const * /*range*/)
{
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		QColor color = getLengthColor(ctFun, objID);
		setObjectColor(objID, color);
	}
	emit dataChanged();
}

void iAColoredPolyObjectVis::setObjectColor(IndexType objIdx, QColor const & qcolor)
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
	auto const pntCnt = finalPolyData() ? &iAColoredPolyObjectVis::finalObjectPointCount
										: &iAColoredPolyObjectVis::objectPointCount;
	auto const startPntIdx = finalPolyData() ? &iAColoredPolyObjectVis::finalObjectStartPointIdx
											 : &iAColoredPolyObjectVis::objectStartPointIdx;
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

void iAColoredPolyObjectVis::setSelectionOpacity(int selectionAlpha)
{
	m_selectionAlpha = selectionAlpha;
}

void iAColoredPolyObjectVis::setContextOpacity(int contextAlpha)
{
	m_contextAlpha = contextAlpha;
}

void iAColoredPolyObjectVis::setupOriginalIds()
{
	auto ids = vtkSmartPointer<vtkIdTypeArray>::New();
	ids->SetName("OriginalIds");
	ids->SetNumberOfTuples(allPointCount());
	for (vtkIdType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		for (vtkIdType pt = 0; pt < objectPointCount(objID); ++pt)
		{
			ids->SetTuple1(objectStartPointIdx(objID) + pt, objID);
		}
	}
	polyData()->GetPointData()->AddArray(ids);
}

void iAColoredPolyObjectVis::setupColors()
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

double const * iAColoredPolyObjectVis::bounds()
{
	return polyData()->GetBounds();
}

void iAColoredPolyObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	m_selection = sortedSelInds;
	m_selectionActive = selectionActive;
	updateColorSelectionRendering();
}

std::vector<size_t> const& iAColoredPolyObjectVis::selection() const
{
	return m_selection;
}

void iAColoredPolyObjectVis::setColor(QColor const &color)
{
	m_baseColor = color;
	m_colorParamIdx = -1;
	m_lut.reset();
	updateColorSelectionRendering();
}

void iAColoredPolyObjectVis::setLookupTable(std::shared_ptr<iALookupTable> lut, size_t paramIndex)
{
	m_lut = lut;
	m_colorParamIdx = paramIndex;
	updateColorSelectionRendering();
}

void iAColoredPolyObjectVis::updateColorSelectionRendering()
{
	size_t curSelIdx = 0;
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		QColor color = m_baseColor;
		if (m_lut)
		{
			double curValue = m_data->m_table->GetValue(objID, m_colorParamIdx).ToDouble();
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

iAColoredPolyObjectVis::IndexType iAColoredPolyObjectVis::objectPointCount(IndexType /*ptIdx*/) const
{
	return DefaultPointsPerObject;
}

iAColoredPolyObjectVis::IndexType iAColoredPolyObjectVis::objectStartPointIdx(IndexType ptIdx) const
{
	return ptIdx * DefaultPointsPerObject;
}

iAColoredPolyObjectVis::IndexType iAColoredPolyObjectVis::allPointCount() const
{
	IndexType pointCount = 0;
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		pointCount += objectPointCount(objID);
	}
	return pointCount;
}

iAColoredPolyObjectVis::IndexType iAColoredPolyObjectVis::finalObjectPointCount(IndexType ptIdx) const
{
	return objectPointCount(ptIdx);
}

iAColoredPolyObjectVis::IndexType iAColoredPolyObjectVis::finalObjectStartPointIdx(IndexType ptIdx) const
{
	return objectStartPointIdx(ptIdx);
}

iAColoredPolyObjectVis::IndexType iAColoredPolyObjectVis::finalAllPointCount() const
{
	IndexType pointCount = 0;
	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID)
	{
		pointCount += finalObjectPointCount(objID);
	}
	return pointCount;
}

/*
vtkAlgorithmOutput* iAColoredPolyObjectVis::output()
{
	return nullptr;
}
*/

std::shared_ptr<iAObjectVisActor> iAColoredPolyObjectVis::createActor(vtkRenderer* ren)
{
	return createPolyActor(ren);
}

std::shared_ptr<iAPolyObjectVisActor> iAColoredPolyObjectVis::createPolyActor(vtkRenderer* ren)
{
	auto result = std::make_shared<iAPolyObjectVisActor>(ren, this);
	connect(this, &iAObjectVis::dataChanged, result.get(), &iAPolyObjectVisActor::updateMapper);
	connect(this, &iAObjectVis::renderRequired, result.get(), &iAPolyObjectVisActor::updateRenderer);
	return result;
}
