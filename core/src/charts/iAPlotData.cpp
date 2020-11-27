#include "iAPlotData.h"

#include "iAMathUtility.h"

#include <QString>

iAPlotData ::~iAPlotData()
{
}

iAValueType iAPlotData::valueType() const
{
	return iAValueType::Continuous;
}

QString iAPlotData::toolTipText(double dataX) const
{
	size_t idx = nearestIdx(dataX);
	double bStart = xValue(idx);
	double bEnd = xValue(idx + 1);
	if (valueType() == iAValueType::Discrete || valueType() == iAValueType::Categorical)
	{
		bStart = static_cast<int>(bStart);
		bEnd = static_cast<int>(bEnd - 1);
	}
	double freq = yValue(idx);
	return QString("%1-%2: %3").arg(bStart).arg(bEnd).arg(freq);
}

size_t iAPlotData::nearestIdx(double dataX) const
{
	double binRng[2] = { 0, static_cast<double>(valueCount()) };
	return clamp(static_cast<size_t>(0), valueCount() - 1, static_cast<size_t>(mapValue(xBounds(), binRng, dataX)));
}
