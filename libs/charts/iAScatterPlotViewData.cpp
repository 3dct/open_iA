// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAScatterPlotViewData.h"

#include "iALog.h"
#include "iAMathUtility.h"

#include "iASPLOMData.h"

iAScatterPlotViewData::iAScatterPlotViewData() :
	m_animIn(1.0),
	m_animOut(0.0),
	m_animationIn(this, "m_animIn"),
	m_animationOut(this, "m_animOut"),
	m_isAnimated(true)
{
	const int animDurationMSec = 100;
	m_animationIn.setDuration(animDurationMSec);
	m_animationOut.setDuration(animDurationMSec);
}

iAScatterPlotViewData::SelectionType& iAScatterPlotViewData::selection()
{
	return m_selection;
}

iAScatterPlotViewData::SelectionType const& iAScatterPlotViewData::selection() const
{
	return m_selection;
}

iAScatterPlotViewData::SelectionType const& iAScatterPlotViewData::filteredSelection(std::shared_ptr<iASPLOMData> splomData) const
{
	if (!filterDefined() || selection().size() == 0)
	{
		m_filteredSelection = m_selection;
		return m_filteredSelection;
	}
	m_filteredSelection.clear();
	size_t curFilteredIdx = 0;
	size_t curSelIdx = 0;
	for (size_t curIdx = 0; curIdx < splomData->numPoints(); ++curIdx)
	{
		if (!matchesFilter(splomData, curIdx))
		{
			continue;
		}
		if (curSelIdx >= m_selection.size())
		{
			break;
		}
		if (curIdx == m_selection[curSelIdx])
		{
			m_filteredSelection.push_back(curFilteredIdx);
			++curSelIdx;
		}
		++curFilteredIdx;
	}
	return m_filteredSelection;
}

void iAScatterPlotViewData::setFilteredSelection(iAScatterPlotViewData::SelectionType const& filteredSelection, std::shared_ptr<iASPLOMData> splomData)
{
	if (!filterDefined())
	{
		setSelection(filteredSelection);
		return;
	}
	std::vector<size_t> sortedFilteredSelInds = filteredSelection;
	std::sort(sortedFilteredSelInds.begin(), sortedFilteredSelInds.end());
	size_t curFilteredIdx = 0, curSelIdx = 0;
	m_selection.clear();
	for (size_t curIdx = 0; curIdx < splomData->numPoints(); ++curIdx)
	{
		if (!matchesFilter(splomData, curIdx))
		{
			continue;
		}
		if (curSelIdx >= sortedFilteredSelInds.size())
		{
			break;
		}
		if (curFilteredIdx == sortedFilteredSelInds[curSelIdx])
		{
			m_selection.push_back(curIdx);
			++curSelIdx;
		}
		++curFilteredIdx;
	}
	emit updateRequired();
}

void iAScatterPlotViewData::setSelection(SelectionType const& selection)
{
	m_selection = selection;
	std::sort(m_selection.begin(), m_selection.end());
	emit updateRequired();
}

void iAScatterPlotViewData::clearSelection()
{
	m_selection.clear();
}

iAScatterPlotViewData::SelectionType const& iAScatterPlotViewData::highlightedPoints() const
{
	return m_highlight;
}

bool iAScatterPlotViewData::isPointHighlighted(size_t idx) const
{
	return std::find(m_highlight.begin(), m_highlight.end(), idx) != m_highlight.end();
}

void iAScatterPlotViewData::addHighlightedPoint(size_t idx)
{
	if (std::find(m_highlight.begin(), m_highlight.end(), idx) == m_highlight.end())
	{
		m_highlight.push_back(idx);
		emit updateRequired();
	}
}

void iAScatterPlotViewData::removeHighlightedPoint(size_t idx)
{
	auto it = std::find(m_highlight.begin(), m_highlight.end(), idx);
	if (it != m_highlight.end())
	{
		m_highlight.erase(it);
		emit updateRequired();
	}
}

void iAScatterPlotViewData::clearHighlightedPoints()
{
	if (m_highlight.size() > 0)
	{
		m_highlight.clear();
		emit updateRequired();
	}
}

bool iAScatterPlotViewData::isInverted(size_t paramIndex)
{
	return paramIndex < m_inverted.size() ? m_inverted[paramIndex] : false;
}

void iAScatterPlotViewData::setInverted(size_t paramIndex, bool isInverted)
{
	m_inverted.resize(paramIndex);
	m_inverted[paramIndex] = isInverted;
}

double iAScatterPlotViewData::animIn() const
{
	return m_animIn;
}

double iAScatterPlotViewData::animOut() const
{
	return m_animOut;
}

iAScatterPlotViewData::LineListType const& iAScatterPlotViewData::lines() const
{
	return m_lines;
}

void iAScatterPlotViewData::addLine(SelectionType const& linePoints, QColor const& color, int lineWidth)
{
	m_lines.push_back(std::make_tuple(linePoints, color, lineWidth));
}

void iAScatterPlotViewData::clearLines()
{
	m_lines.clear();
}

void iAScatterPlotViewData::setAnimIn(double anim)
{
	m_animIn = anim;
	emit updateRequired();
}

void iAScatterPlotViewData::setAnimOut(double anim)
{
	m_animOut = anim;
	emit updateRequired();
}

void iAScatterPlotViewData::updateAnimation(size_t curPt, size_t prePt)
{
	if (!m_isAnimated)
	{
		return;
	}
	if (prePt != iASPLOMData::NoDataIdx && curPt != prePt)
	{
		m_animationOut.setStartValue(m_animIn);
		m_animationOut.setEndValue(0.0);
		m_animationOut.start();
	}
	if (curPt != iASPLOMData::NoDataIdx)
	{
		const double animStart = 0.0;
		m_animationIn.setStartValue(animStart);
		m_animationIn.setEndValue(1.0);
		m_animationIn.start();
	}
}

bool iAScatterPlotViewData::matchesFilter(std::shared_ptr<iASPLOMData> splomData, size_t ind) const
{
	if (m_filters.empty())
	{
		return true;
	}
	for (auto filter : m_filters)
	{
		if (filter.first >= splomData->numParams())
		{
			LOG(lvlWarn, QString("Invalid filter column ID %1 (>= column count %2)!")
				.arg(filter.first).arg(splomData->numParams()));
			return false;
		}
		if (dblApproxEqual(splomData->paramData(filter.first)[ind], filter.second))
		{
			return true;
		}
	}
	return false;
}

void iAScatterPlotViewData::addFilter(size_t paramIndex, double value)
{
	m_filters.push_back(std::make_pair(paramIndex, value));
	emit filterChanged();
}

void iAScatterPlotViewData::removeFilter(size_t paramIndex, double value)
{
	auto searchedPair = std::make_pair(paramIndex, value);
	auto it = std::find(m_filters.begin(), m_filters.end(), searchedPair);
	if (it != m_filters.end())
	{
		m_filters.erase(it);
	}
	emit filterChanged();
}

void iAScatterPlotViewData::clearFilters()
{
	m_filters.clear();
	emit filterChanged();
}

bool iAScatterPlotViewData::filterDefined() const
{
	return m_filters.size() > 0;
}
