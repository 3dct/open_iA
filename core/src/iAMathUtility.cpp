/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAMathUtility.h"

#include <algorithm>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif


double gaussian(double x, double sigma)
{
	return 1 / std::sqrt(2 * M_PI*std::pow(sigma, 2.0)) *
		std::exp(-std::pow(x, 2.0) / (2 * std::pow(sigma, 2.0)));
}


std::vector<double> gaussianKernel(double kernelSigma, size_t kernelSteps)
{
	std::vector<double> kernel;
	for (size_t i = 0; i < kernelSteps + 1; ++i)
	{
		kernel.push_back(gaussian(i, kernelSigma));
	}
	return kernel;
}

//! convolutes the given function with a Gaussian kernel with the given sigma and steps
//! TODO: steps could be  calculated from sigma (cut off kernel when factor gets very small
std::vector<double> gaussianSmoothing(std::vector<double> data, double kernelSigma, size_t kernelSteps)
{
	std::vector<double> smoothed;
	auto kernel = gaussianKernel(kernelSigma, kernelSteps);
	for (size_t i = 0; i < data.size(); ++i)
	{
		size_t minIdx = std::max(static_cast<size_t>(0), i - kernelSteps);
		size_t maxIdx = std::min(data.size(), i + kernelSteps + 1);
		double smoothValue = 0;
		for (size_t cur = minIdx; cur < maxIdx; ++cur)
		{						// kernel is symmetric around 0
			size_t kernelIdx = std::abs(static_cast<long long>(cur - i));
			assert(kernelIdx < kernel.size());
			smoothValue += kernel[kernelIdx] * data[i];
		}
		smoothValue /= (maxIdx - minIdx + 1);
		smoothed.push_back(smoothValue);
	}
	return smoothed;
}
