// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iacharts_export.h"

#include "iAPlotData.h"

#include <QSharedPointer>

#include <utility> // for pair
#include <vector>

//! Holds data of a plot in the form of (x,y) value pairs.
class iAcharts_API iAXYPlotData : public iAPlotData
{
public:
	iAXYPlotData(QString const& name, iAValueType type, size_t reservedSize);
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
	std::vector<std::pair<DataType, DataType>> m_values;
	DataType m_xBounds[2], m_yBounds[2];
};
