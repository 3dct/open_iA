#pragma once

#include <map>
#include <string>


class iAQMeasureCalculation
{

public:

static std::map<std::string, double> computeOrigQ(
		float* fImage, const int* dim, const double* range, int HistogramBins, int NumberPeaks, bool AnalyzePeak);
};