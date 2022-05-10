/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed

/**************************Define histogram type**********************************/
using axes_regular = std::tuple<
	boost::histogram::axis::regular<>
>;
using regularStatic1DHistogram = boost::histogram::histogram<axes_regular>;
/*********************************************************************************/

struct HistogramParameters
{
	int featureID = -1;
	double bins = 0;
	double binWidth = 0;
	//Holds the observations for every [bin]
	std::vector<int> observations = std::vector<int>();
	double maxValue = 0;
	double minValue = 0;
	double mean = 0;
	double median = 0;
	double standartDeviation = 0;
	double skewness = 0;
	double kurtosis = 0;
	double modality = 0;
};

//! Stores Histogram values
class iAVRHistogram
{
public:
	iAVRHistogram();
	HistogramParameters m_histogramParameters;

	regularStatic1DHistogram* getHistogram();

private:
	regularStatic1DHistogram m_histogram;

	void initializeDataStructure();
};
