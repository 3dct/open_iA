#include "iAPlotData.h"

#include "iAMathUtility.h"

#include <QString>


iAPlotData::iAPlotData(iAValueType type): m_valueType(type)
{
}

iAPlotData::~iAPlotData()
{
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
