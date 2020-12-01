/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "open_iA_Core_export.h"

#include "iAPlotData.h"

#include <QSharedPointer>

#include <utility> // for pair
#include <vector>

class open_iA_Core_API iAXYPlotData : public iAPlotData
{
public:
	//! @{
	//! overriding methods from iAPlotData
	DataType xValue(size_t idx) const override;
	DataType yValue(size_t idx) const override;
	DataType const* xBounds() const override;
	DataType const* yBounds() const override;
	size_t valueCount() const override;
	size_t nearestIdx(DataType dataX) const override;
	QString toolTipText(DataType dataX) const override;

	//! Adds a new x/y pair. Note that entries need to be added in order of their x component
	void addValue(DataType x, DataType y);
	//! Create an empty data object
	static QSharedPointer<iAXYPlotData> create(QString const& name, iAValueType type, size_t reservedSize);

private:
	iAXYPlotData(QString const& name, iAValueType type, size_t reservedSize);
	std::vector<std::pair<DataType, DataType>> m_values;
	DataType m_xBounds[2], m_yBounds[2];
};