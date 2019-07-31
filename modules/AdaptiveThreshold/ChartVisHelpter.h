#pragma once

using namespace QtCharts;

class QtCharts::QXYSeries;
class ParametersRanges; 

class ChartVisHelpter
{

	static QXYSeries* create(const ParametersRanges& ranges); 

};

