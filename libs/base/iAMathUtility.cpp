// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMathUtility.h"

// TODO: on switch to C++20, replace vtkMath::Pi usage with std::numbers::pi
// see https://stackoverflow.com/a/57285400/671366
// would make linking to vtk libraries in test also superfluous, see also other refs to vtkMath::Pi() throughout codebase!
#include <vtkMath.h>

double gaussian(double x, double sigma)
{
	return 1 / std::sqrt(2 * vtkMath::Pi()*std::pow(sigma, 2.0)) *
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

std::vector<double> gaussianSmoothing(std::vector<double> const & data, double kernelSigma, int kernelSteps)
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

std::vector<double> derivative(std::vector<double> const & func)
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

double mean(FuncType const & func)
{
	double sum = std::accumulate(func.begin(), func.end(), 0.0);
	return sum / func.size();
}

double variance(FuncType const & func, double meanVal, bool correctDF)
{
	if (std::isinf(meanVal))
		meanVal = mean(func);
	double sq_sum = std::inner_product(func.begin(), func.end(), func.begin(), 0.0,
		[](double const & x, double const & y) { return x + y; },
		[meanVal](double const & x, double const & y) { return (x - meanVal)*(y - meanVal); });
	return sq_sum / (func.size() - (correctDF? 1 : 0) );
}

double standardDeviation(FuncType const & func, double meanVal, bool correctDF)
{
	return std::sqrt(variance(func, meanVal, correctDF));
}

double covariance(FuncType const & func1, FuncType const & func2, double mean1, double mean2, bool correctDF)
{
	if (std::isinf(mean1))
		mean1 = mean(func1);
	if (std::isinf(mean2))
		mean2 = mean(func2);
	double sq_sum = std::inner_product(func1.begin(), func1.end(), func2.begin(), 0.0,
		[](double const & x, double const & y) { return x + y; },
		[mean1, mean2](double const & x, double const & y) { return (x - mean1)*(y - mean2); });
	return sq_sum / (func1.size() - (correctDF ? 1 : 0) );
}

double pearsonsCorrelationCoefficient(FuncType const & func1, FuncType const & func2)
{
	double mean1 = mean(func1), mean2 = mean(func2);
	double stddev1 = standardDeviation(func1, mean1), stddev2 = standardDeviation(func2, mean2);
	double cov = covariance(func1, func2, mean1, mean2);
	return cov / (stddev1 * stddev2);
}

FuncType getNormedRanks(FuncType const& func)
{
	auto sortIdx = sort_indexes(func);
	auto idxIt = sortIdx.cbegin();
	FuncType result (func.size());
	while (idxIt != sortIdx.cend())
	{
		auto idxSameRankStart = idxIt;
		while (idxIt + 1 != sortIdx.cend() && (func[*(idxIt+1)] == func[*idxIt]))
		{
			++idxIt;
		}
		// rank should start at 1; our indices start at 0, therefore +1 below
		size_t startRank = idxSameRankStart - sortIdx.cbegin() + 1;
		size_t numWithSameRank = idxIt - idxSameRankStart + 1;
		double normRank = startRank + (numWithSameRank - 1) / 2.0;
		for (size_t outOfs = 0; outOfs < numWithSameRank; ++outOfs)
		{
			result[*(idxSameRankStart + outOfs)] = normRank;
		}
		++idxIt;
	}
	return result;
}

double spearmansCorrelationCoefficient(FuncType const& func1, FuncType const& func2)
{
	assert(func1.size() == func2.size());
	auto normedRanks1 = getNormedRanks(func1);
	auto normedRanks2 = getNormedRanks(func2);
	return pearsonsCorrelationCoefficient(normedRanks1, normedRanks2);
}
