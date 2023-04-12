// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>    // for size_t
#include <utility>    // for std::pair
#include <vector>

#include <QColor>
#include <QObject>
#include <QPropertyAnimation>
#include <QSharedPointer>

#include "iAcharts_export.h"


class iASPLOMData;

//! Class providing details on the current viewing configuration of a scatterplot.
//! Works with both a single scatterplot or a scatterplot matrix (iAScatterPlotWidget or iAQSplom).
//! This includes details on the current selection, highlight, mouse hover animation, and lines connecting points.
class iAcharts_API iAScatterPlotViewData : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double m_animIn READ animIn WRITE setAnimIn)
	Q_PROPERTY(double m_animOut READ animOut WRITE setAnimOut)
public:
	iAScatterPlotViewData();

	using SelectionType = std::vector<size_t>;
	using LineType = std::tuple<iAScatterPlotViewData::SelectionType, QColor, int>;
	using LineListType = std::vector<LineType>;

	SelectionType& selection();
	SelectionType const& selection() const;
	void setSelection(SelectionType const& selection);
	//! returns the index of the selected points in the filtered list of points
	//! i.e. the index of those points that are selected in a list which only contains those points which match the current filter
	//! NOTE: Only useful if you actually have such a filtered list! As is e.g. currently created in iAScatterPlot::drawPoints when SP_OLDOPENGL is defined...
	SelectionType const& filteredSelection(QSharedPointer<iASPLOMData> splomData) const;
	void setFilteredSelection(SelectionType const& filteredSelection, QSharedPointer<iASPLOMData> splomData);
	void clearSelection();

	SelectionType const& highlightedPoints() const;
	bool isPointHighlighted(size_t idx) const;
	void addHighlightedPoint(size_t idx);
	void removeHighlightedPoint(size_t idx);
	void clearHighlightedPoints();

	//!< Get whether the axis of a parameter should be inverted in the scatter plots.
	bool isInverted(size_t paramIndex);
	//!< Set whether the axis of a parameter should be inverted in the scatter plots.
	void setInverted(size_t paramIndex, bool isInverted);

	//! @{
	//! Connecting lines between data points
	LineListType const& lines() const;
	void addLine(SelectionType const& linePoints, QColor const& color, int lineWidth);
	void clearLines();
	//! @}
	
	//! @{
	//! Filtering for data items (matching values)
	bool matchesFilter(QSharedPointer<iASPLOMData> splomData, size_t ind) const; //!< Returns true if point with given index matches current filter
	void addFilter(size_t paramIndex, double value);  //!< Adds a filter on the data to be shown, on the given column (index). The value in this column needs to match the given value; multiple filters added via this function are linked via OR.
	void removeFilter(size_t paramIndex, double value);//!< Removes the filter on the given column and value.
	void clearFilters();                              //!< Clear all filters on data; after calling this method, all data points will be shown again.
	bool filterDefined() const;                       //!< Returns true if a filter is defined on the data
	//! @}

	double animIn() const;         //!< Getter for animation in property
	void setAnimIn(double anim);   //!< Setter for animation in property
	double animOut() const;        //!< Getter for animation out property
	void setAnimOut(double anim);  //!< Setter for animation out property
	void updateAnimation(size_t curPt, size_t prePt);
signals:
	void updateRequired();
	void filterChanged();  //!< emitted whenever a filter is added, removed, or all filters cleared
private:
	//!< contains indices of highlighted (clicked) points
	SelectionType m_highlight;
	//!< contains indices of currently selected data points
	SelectionType m_selection;
	//!< contains indices of selected points in filtered list (TODO: update only when selection changes and when filters change, remove mutable)
	mutable SelectionType m_filteredSelection;
	//!< whether to invert a feature
	std::vector<char> m_inverted;
	//!< indices of pairs of points which should be connected by a line of the given color
	LineListType m_lines;
	double m_animIn;   //!< In animation parameter
	double m_animOut;  //!< Out animation parameter
	QPropertyAnimation m_animationIn;
	QPropertyAnimation m_animationOut;
	// settings:
	bool m_isAnimated;

	//! collection of filters: each column index/value pair is linked via OR
	std::vector<std::pair<size_t, double> > m_filters;
};
