// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPlotData.h"

#include <QString>


iAPlotData::iAPlotData(QString const & name, iAValueType type): m_name(name), m_valueType(type)
{}

iAPlotData::~iAPlotData()
{}

QString const& iAPlotData::name() const
{
	return m_name;
}

iAValueType iAPlotData::valueType() const
{
	return m_valueType;
}

void adaptBounds(iAPlotData::DataType bounds[2], iAPlotData::DataType value)
{
	if (value < bounds[0])
	{
		bounds[0] = value;
	}
	if (value > bounds[1])
	{
		bounds[1] = value;
	}
}
