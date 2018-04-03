/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

double gaussian(double x, double sigma)
{
	return 1 / std::sqrt(2 * Pi*std::pow(sigma, 2.0)) *
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

std::vector<double> gaussianSmoothing(std::vector<double> data, double kernelSigma, int kernelSteps)
{
	std::vector<double> smoothed;
	auto kernel = gaussianKernel(kernelSigma, kernelSteps);
	for (size_t i = 0; i < data.size(); ++i)
	{
		size_t minIdx = static_cast<size_t>(std::max(static_cast<long long>(0), static_cast<long long>(i) - kernelSteps));
		size_t maxIdx = std::min(data.size(), i + kernelSteps + 1);
		double smoothValue = 0;
		for (size_t cur = minIdx; cur < maxIdx; ++cur)
		{						// kernel is symmetric around 0
			size_t kernelIdx = std::abs(static_cast<long long>(cur - i));
			assert(kernelIdx < kernel.size());
			smoothValue += kernel[kernelIdx] * data[cur];
		}
		smoothed.push_back(smoothValue);
	}
	return smoothed;
}

open_iA_Core_API std::vector<double> derivative(std::vector<double> func)
{
	if (func.size() <= 1)
		return func;
	std::vector<double> deriv;
	for (size_t i = 0; i < func.size(); ++i)
	{
		double derivValue = ((i > 0) ? (func[i] - func[i - 1])
			: func[i+1]-func[i]);
			// calculate as average of(difference between prev and current) and (difference between current and next) ?
			//(i < func.size()-1) ? (func[i+1] - func[i])
		deriv.push_back(derivValue);
	}
	return deriv;
}
