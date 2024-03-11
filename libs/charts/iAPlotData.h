// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAValueType.h"

#include "iacharts_export.h"

#include <QString>

#include <cstddef> // for size_t

//! Abstract base class providing data used for drawing a plot in a chart.
class iAcharts_API iAPlotData
{
public:
	//! Data type used for x and y values.
	using DataType=double;
	//! Construct a plot data object.
	//! @param name the name of the data series that this object holds.
	//! @param type  type of the values held by this data object, see iAValueType
	iAPlotData(QString const & name, iAValueType type);
	virtual ~iAPlotData();
	//! The name of the data series that this object holds.
	QString const& name() const;
	//! The type of the values held by this data object.
	virtual iAValueType valueType() const;
	//! Value on the x axis for a datum with given index.
	//! @param idx data index
	virtual DataType xValue(size_t idx) const = 0;
	//! Value on the y axis for a datum with given index.
	//! @param idx data index
	virtual DataType yValue(size_t idx) const = 0;
	//! The range of values for x; i.e. xBounds()[0] is the minimum of all xValue(...), xBounds()[1] is the maximum.
	virtual DataType const* xBounds() const = 0;
	//! The range of values for y; i.e. yBounds()[0] is the minimum of all yValue(...), yBounds()[1] is the maximum.
	virtual DataType const* yBounds() const = 0;
	//! The number of available data elements (i.e. in xValue/yValue, idx can go from 0 to valueCount()-1).
	virtual size_t valueCount() const = 0;
	//! Retrieve the index closest to the given x data value.
	//! @param dataX the value (on the x axis) for which to search the closest datapoint in this data object.
	//! @return the index (such as can be passed to xValue/yValue) of the datapoint closest to dataX
	//!         calling xValue on the returned index will always give a value lower than or equal to dataX.
	virtual size_t nearestIdx(DataType dataX) const = 0;

	//! The tooltip text for this data when the user is currently hovering over the given x position.
	//! Note that currently only the x axis position of the user is considered.
	//! @param dataX the value (on the x axis) the user currently is hovering over.
	//! @return a description of the datapoint that the user currrently is over/closest to.
	virtual QString toolTipText(DataType dataX) const =0;

private:
	//! The name of the data series that this object holds.
	QString m_name;
	//! The type of the values that were used as input to compute the histogram.
	iAValueType m_valueType;
};

//! helper function for adapting bounds to a given value
void adaptBounds(iAPlotData::DataType bounds[2], iAPlotData::DataType value);
