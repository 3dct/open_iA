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

double iAPlotData::binStart(size_t binNr) const  // default: assume constant (i.e. linear) spacing
{
	return spacing() * binNr + xBounds()[0];
}

QString iAPlotData::toolTipText(double dataX) const
{
	size_t binNr = dataX2Bin(dataX);
	double bStart = binStart(binNr);
	double bEnd = binStart(binNr + 1);
	if (valueType() == iAValueType::Discrete || valueType() == iAValueType::Categorical)
	{
		bStart = static_cast<int>(bStart);
		bEnd = static_cast<int>(bEnd - 1);
	}
	double freq = rawData()[binNr];
	return QString("%1-%2: %3").arg(bStart).arg(bEnd).arg(freq);
}

size_t iAPlotData::dataX2Bin(double dataX) const
{
	double binRng[2] = { 0, static_cast<double>(numBin()) };
	return clamp(static_cast<size_t>(0), numBin() - 1, static_cast<size_t>(mapValue(xBounds(), binRng, dataX)));
}