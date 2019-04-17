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
#pragma once

#include "iAColorable.h"
#include "open_iA_Core_export.h"

#include <QSharedPointer>

class iAPlotData;
class iAMapper;

class QColor;
class QPainter;

/** Interface for a function which is drawable in a diagram
 *	encapsulates both the data of the function and the drawing method */
class open_iA_Core_API iAPlot: public iAColorable
{
public:
	iAPlot(QSharedPointer<iAPlotData> data, QColor const & color);
	virtual ~iAPlot();
	//! Draws the plot
	virtual void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const = 0;
	//! Retrieves the data used for drawing
	virtual QSharedPointer<iAPlotData> data();
	//! Whether the plot is currently being drawn
	virtual bool visible() const;
	//! Sets whether the plot should be currently drawn or not
	virtual void setVisible(bool visible);
protected:
	QSharedPointer<iAPlotData> m_data;
	bool m_visible;
};
