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

//! Class providing details on the current viewing configuration
//! to a scatterplot or a scatterplot matrix (iAScatterPlotWidget or iAQSplom).
//! This includes details on the current selection, highlight,
//! mouse hover animation, and lines connecting points.
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

	LineListType const& lines() const;
	void addLine(SelectionType const& linePoints, QColor const& color, int lineWidth);
	void clearLines();

	double animIn() const;         //!< Getter for animation in property
	void setAnimIn(double anim);   //!< Setter for animation in property
	double animOut() const;        //!< Getter for animation out property
	void setAnimOut(double anim);  //!< Setter for animation out property
	void updateAnimation(size_t curPt, size_t prePt);
signals:
	void updateRequired();
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
};
