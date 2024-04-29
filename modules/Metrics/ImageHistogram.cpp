// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ImageHistogram.h"


#include <numbers> // for Pi

#include <algorithm>
#include <cmath>
#include <cassert>

namespace
{
	const double DoubleEpsilon = 2.2204460492503131e-016; /* smallest such that 1.0+DoubleEpsilon != 1.0 */
	bool DoubleEquals(double x, double v)
	{
		return ((v - DoubleEpsilon) < x) && (x < (v + DoubleEpsilon));
	}

	const double LOG2M1 = 1.4426950408889634073599246810018921374266459541529860;
	// Calculates log2=log(n)/log(2)
	double dlog2(double n)
	{
		return std::log(n)*LOG2M1;
	};

	bool HistPosSortIncrIDX(cImageHistogram::HistPos a, cImageHistogram::HistPos b)	// Sort HistPos by IDX from lowest to highest
	{
		return a.idx < b.idx;
	}

	bool HistPosSortDecrY(cImageHistogram::HistPos a, cImageHistogram::HistPos b)		// Sort HistPos by y from highest to lowest
	{
		return a.y > b.y;
	}

	bool dHistPosSortIncrY(cImageHistogram::dHistPos a, cImageHistogram::dHistPos b)	// Sort HistPos by y from lowest to highest
	{
		return a.y < b.y;
	}
}

// cImageHistogram
// ===========================================================================

int cImageHistogram::CreateHist(float* fImage, unsigned int nPixelH, unsigned int nPixelW, unsigned int nPixelD, int n, float min, float max, bool ConsiderRecoFillZeroes, unsigned long long nCutL, unsigned long long nCutH)
{
	assert(n >= 0);
	bins = n;
	unsigned long long datasize = (unsigned long long)nPixelH*nPixelW*nPixelD;
	float curX, StepPerBin;
	std::vector<unsigned long long> trunc_idx; trunc_idx.push_back(0); trunc_idx.push_back(n-1);

	// Two iterations are only needed if several counts should be cuted from beginning and end of the first iterations histogram
	int runs=1;
	if (((nCutL > 0) || (nCutH > 0)) && (nCutL + nCutH) < datasize)
	{
		runs = 2;
	}
	for (int r=0; r<runs; r++)
	{
		if(r==1)
		{
			min = hist_x[trunc_idx[0]];
			max = hist_x[trunc_idx[1]];
		}

		// Generate x axes and reset y axes
		curX = min;
		StepPerBin = (max-min)/(bins-1);
		hist_y.clear();
		hist_x.clear();
		for(int i=0; i<bins; i++)
		{
			hist_y.push_back(0);
			hist_x.push_back(curX);
			curX+=StepPerBin;
		}
		int zero_idx = static_cast<int>(std::floor(((0.0f-min)/StepPerBin)+0.5f));

		// Calculate histogram by indexing
		for(unsigned long long index=0; index<datasize; index++)
		{
			curX = std::floor(((fImage[index]-min)/StepPerBin)+0.5f);
			if (curX < 0)
			{
				hist_y[0]++;
			}
			else if (curX > n - 1)
			{
				hist_y[n - 1]++;
			}
			else
			{
				hist_y[(int)curX]++;
			}
		}

		// Calculate new min/max values with truncation
		if (r==0 && runs==2)
		{
			// Accumulate data to the lower truncation index
			unsigned long long sum=0;
			for (int t=0; t < n; ++t)
			{
				sum += hist_y[t];
				if(sum > nCutL)
				{
					if (t > 0)
					{
						trunc_idx[0] = t - 1;
					}
					break;
				}
			}

			// Accumulate data to the upper truncation index
			sum=0;
			for(int t=n-1; t>=0; --t)
			{
				sum += hist_y[t];
				if (sum > nCutH)
				{
					if (t < n - 1)
					{
						trunc_idx[1] = t + 1;
					}
					break;
				}
			}
		}
		else
		{
			// Handle reco fill zeroes
			if (!ConsiderRecoFillZeroes && zero_idx == 0)
			{
				hist_y[zero_idx] = hist_y[1];
			}
			else if (!ConsiderRecoFillZeroes && zero_idx == bins - 1)
			{
				hist_y[zero_idx] = hist_y[bins - 2];
			}
			else if (!ConsiderRecoFillZeroes && zero_idx > 0 && zero_idx < bins - 1)
			{
				hist_y[zero_idx] = (unsigned long long)(((double)hist_y[zero_idx - 1] + hist_y[zero_idx + 1])*0.5 + 0.5);
			}
		}
	}

	return (int)hist_y.size();
}

unsigned int cImageHistogram::DetectPeaksValleys(unsigned int nPeaks, unsigned int dgauss_size_BINscale, unsigned int gauss_size_P2Pscale, double threshold_x , double threshold_y, bool consTruncBin)
{
	std::vector<double> gauss_knl, dgauss_knl;

	// Generate Gauss kernel
	// =========================================================================
	int dknl_sz = (int)std::floor((0.5f*bins/dgauss_size_BINscale+0.5f));
	if(dknl_sz<1)
		dknl_sz = 1;                 // y' via gauss': [1.184 0 -1.184] ~= diff: [1 -1]
	float gauss_sigma = (float)dknl_sz/4;

	double sum=0.0;
	for(int i=-dknl_sz; i<=dknl_sz; i++)
	{
		double value =
			1 / (std::sqrt(2 * std::numbers::pi) * gauss_sigma) * std::exp(-0.5 * i * i / (gauss_sigma * gauss_sigma));
		gauss_knl.push_back(value);
		sum+=value;
	}
	for(std::vector<double>::iterator iter=gauss_knl.begin(); iter<gauss_knl.end(); iter++)
		*iter /= sum;

	// Generate derivated Gauss kernel
	// =========================================================================
	for(int i=0; i<dknl_sz*2; i++)
		dgauss_knl.push_back(gauss_knl[i+1]-gauss_knl[i]);

	// Derivate histogram by convoluting with an derivated gauss
	// =========================================================================
	dghist_y = Conv1D(hist_y,dgauss_knl,1);
	dghist_x = hist_x;

	// Find local maxima
	// y' goes through the x-axes from	pos to neg values --> possible maximum
	//									neg to pos values --> possible minimum
	// Skip peaks beyond the given x and y thresholds
	// =========================================================================
	HistPos cur;
	if(consTruncBin)
	{
		if(dghist_y.front()<0 && hist_x.front()>=threshold_x && hist_y.front()>=threshold_y)
		{
			cur.idx = 1;
			cur.x = hist_x.front();
			cur.y = hist_y.front();
			Peaks.push_back(cur);
		}
		if(dghist_y.back()>0 && hist_x.back()>=threshold_x && hist_y.back()>=threshold_y)
		{
			cur.idx = 1;
			cur.x = hist_x.back();
			cur.y = hist_y.back();
			Peaks.push_back(cur);
		}
	}
	for(int i = 1; i<=bins-1; i++)
	{
		if(dghist_y[i-1]>0 && dghist_y[i]<=0 && hist_x[i]>=threshold_x && hist_y[i]>=threshold_y)
		{
			cur.idx = i;
			cur.x = hist_x[i];
			cur.y = hist_y[i];
			Peaks.push_back(cur);
		}
	}

	// Sort HistPos by y from highest to lowest
	std::sort(Peaks.begin(),Peaks.end(),HistPosSortDecrY);

	// Truncate detected Peaks to nPeaks
	for(;Peaks.size()>nPeaks;)
		Peaks.erase(Peaks.end()-1);

	// Sort HistPos by IDX from lowest to highest
	std::sort(Peaks.begin(),Peaks.end(),HistPosSortIncrIDX);

	// Find local minima
	// =========================================================================
	if(Peaks.size()>1)
	{
		for(int p=0; p<(int)Peaks.size()-1; p++)
		{
			// Generate Gauss kernel and smooth histogram by convolution with an gauss
			// between two peaks to find local minima
			// Gauss kernel 1/gauss_sz_scal of peak to peak distance
			// =========================================================================
			std::vector<double> gauss_knl_P2P;
			int knl_sz = (int)std::floor(0.5f*std::abs((float)Peaks[p+1].idx-Peaks[p].idx)/gauss_size_P2Pscale+0.5f);
			if(knl_sz<1)
				knl_sz = 1;                 // y' via gauss': [1.184 0 -1.184] ~= diff: [1 -1]
			float new_gauss_sigma = (float)knl_sz/4;

			sum = 0.0;
			for(int i=-knl_sz; i<=knl_sz; i++)
			{
				double value = 1 / (std::sqrt(2 * std::numbers::pi) * new_gauss_sigma) *
					std::exp(-0.5 * i * i / (new_gauss_sigma * new_gauss_sigma));
				gauss_knl_P2P.push_back(value);
				sum+=value;
			}
			for(std::vector<double>::iterator iter=gauss_knl_P2P.begin(); iter<gauss_knl_P2P.end(); iter++)
				*iter /= sum;

			ghist_y = Conv1D(hist_y,gauss_knl_P2P,1);

			// find local minimum
			double minval=(double)Peaks[p].y;		// or Peaks[i+1].y
			std::vector<int> tmp_idx; tmp_idx.push_back(-1); tmp_idx.push_back(-1);
			for(int i=(int)Peaks[p].idx+1; i<(int)Peaks[p+1].idx; i++)
			{
				if(ghist_y[i] < minval)
				{
					minval = ghist_y[i];
					tmp_idx[0] = i;
					tmp_idx[1] = i;
				}
				if(DoubleEquals(ghist_y[i],minval) )		// if maximum is equal, the mean between the idx is taken
					tmp_idx[1] = i;
			}

			if(tmp_idx[0]>=0 && tmp_idx[1]>=0)
			{
				cur.idx = (int)((tmp_idx[1]+tmp_idx[0])/2);
				cur.x = hist_x[cur.idx];
				cur.y = (unsigned long long)hist_y[cur.idx];
				Valleys.push_back(cur);
			}
		}
	}

	return (int)Peaks.size();
}

float cImageHistogram::CalcQ(std::vector<int> thrsh_IDX, std::vector<ClassMeasure> &result, int Q_equation)
{
	int CStart, CEnd,
		maxprobabilityClassIDX = -1;
	unsigned long long nval;
	double sum, count, counttotal = 0, maxprobability = 0.0;
	ClassMeasure val;
	result.clear();

	// Total count of data points
	for (std::vector<unsigned long long>::iterator iter = hist_y.begin(); iter < hist_y.end(); iter++)
	{
		counttotal += *iter;
	}

	// Sort by increasing indices
	std::sort(thrsh_IDX.begin(), thrsh_IDX.end());

	// Calculate data per class, divided by the thresholds
	assert(thrsh_IDX.size() < std::numeric_limits<int>::max());
	for (int t = 0; t <= static_cast<int>(thrsh_IDX.size()); ++t)
	{
		if (t == 0)
		{
			CStart = 0;
			CEnd = thrsh_IDX[t];
		}
		else if (t == static_cast<int>(thrsh_IDX.size()))
		{
			CStart = thrsh_IDX[t - 1];
			CEnd = static_cast<int>(hist_y.size());
		}
		else
		{
			CStart = thrsh_IDX[t - 1];
			CEnd = thrsh_IDX[t];
		}

		// mean
		count = 0;
		sum = 0;
		nval = 0;
		for (int i = CStart; i < CEnd; i++)
		{
			count += hist_y[i];
			sum += static_cast<double>(hist_y[i])*hist_x[i];
			nval += hist_y[i];
		}
		val.mean = (float)(sum / nval);

		// standard deviation
		sum = 0;
		for (int i = CStart; i < CEnd; i++)
		{
			sum += std::pow(static_cast<double>(hist_x[i]) - val.mean, 2)*hist_y[i];
		}

		val.sigma = (float)std::pow(sum / nval, 0.5);

		//probability
		val.probability = (float)(count / counttotal);

		if (t > 0 && val.probability > maxprobability)
		{
			maxprobability = val.probability;
			maxprobabilityClassIDX = t;
		}
		val.UsedForQ = 0;
		val.LowerThreshold = hist_x[CStart];
		val.UpperThreshold = hist_x[CEnd - 1];
		result.push_back(val);
	}
	result[0].UsedForQ = 1;
	result[maxprobabilityClassIDX].UsedForQ = 2;

	// Total quality measure between the lowest absorbing (air) and the peak with highest probability
	if (Q_equation == 0)
	{
		return std::abs(result[maxprobabilityClassIDX].mean - result[0].mean) / std::sqrt(result[maxprobabilityClassIDX].sigma*result[0].sigma);
	}
	else
	{
		return std::abs(result[maxprobabilityClassIDX].mean - result[0].mean) / std::sqrt(result[maxprobabilityClassIDX].sigma*result[maxprobabilityClassIDX].sigma + result[0].sigma*result[0].sigma);
	}
}

float cImageHistogram::CalcEntropy()
{
	double H=0.0,Hmax=0.0,counttotal=0;

	// Total count of data points
	for (std::vector<unsigned long long>::iterator iter = hist_y.begin(); iter < hist_y.end(); iter++)
	{
		counttotal += *iter;
	}

	// Calculate propabilities
	dHistPosList p;
	dHistPos val;
	int i=1;
	for(std::vector<unsigned long long>::iterator iter = hist_y.begin(); iter < hist_y.end(); iter++)
	{
		val.idx=i;
		val.y=*iter/counttotal;
		p.push_back(val);
		i++;
	}

	// Sort p by y from lowest to highest values to decrease the calculation error of the sum
	std::sort(p.begin(),p.end(),dHistPosSortIncrY);

	// Test sum with and without sort
	//double sss=0;
	//for(dHistPosList::iterator iter = p.begin(); iter < p.end(); iter++)
	//	sss += iter->y;

	// Calculate Relative Shannon Entropy Hmax
	Hmax = dlog2((double)bins);												// Maximal Shannon Entropy Hmax
	for(dHistPosList::iterator iter = p.begin(); iter < p.end(); iter++)
	{
		//double mx = (iter->y*dlog2(iter->y));
		if (iter->y >= DoubleEpsilon)
		{
			H += (iter->y*dlog2(iter->y));
		}
	}
	H/=-Hmax;

	return (float)H;
}

std::vector<int> cImageHistogram::GetValleyThreshold_IDX()
{
	std::vector<int> thrsh;

	for (HistPosList::iterator iter = Valleys.begin(); iter < Valleys.end(); iter++)
	{
		thrsh.push_back(iter->idx);
	}

	return thrsh;
}

std::vector<float> cImageHistogram::GetValleyThreshold()
{
	std::vector<float> thrsh;

	for (HistPosList::iterator iter = Valleys.begin(); iter < Valleys.end(); iter++)
	{
		thrsh.push_back(iter->x);
	}

	return thrsh;
}

std::vector<double> cImageHistogram::Conv1D(std::vector<unsigned long long> in, std::vector<double> knl, int mode)
{
	std::vector<double> out;
	int i, iext, k, kext, extsize = ((int)knl.size()-1), fullsize = (int)in.size()+(int)knl.size()-1, hlpsize;
	if (knl.size() % 2)
	{
		hlpsize = ((int)knl.size() - 1) / 2;
	}
	else
	{
		hlpsize = (int)knl.size() / 2 - 1;
	}

	// Zero padding
	for(i=0; i<extsize; i++)
	{
		in.insert(in.begin(),0);
		in.insert(in.end(),0);
	}

	// Full convolution
	for(i=0, iext=hlpsize; i<fullsize; i++, iext++)
	{
		out.push_back(0.0f);
		for (k = (int)knl.size() - 1, kext = -hlpsize; k >= 0; k--, kext++)
		{
			out[i] += knl[k] * in[iext + kext];
		}
	}

	if(mode==1)
	{
		// Reduce to same (core) size
		if (knl.size() % 2)
		{
			out.erase(out.begin(), out.begin() + hlpsize);
		}
		else
		{
			out.erase(out.begin(), out.begin() + hlpsize + 1);
		}
		out.erase(out.end()-hlpsize,out.end());
	}
	else if(mode==2)
	{
		// Reduce to valid size
		out.erase(out.begin(),out.begin()+extsize);
		out.erase(out.end()-extsize,out.end());
	}

	return out;
}

/*
void cImageHistogram::Conv1D_test()
{
	// test different combinations of odd and even data and kernel sizes, successfull comparion with conv_test.m
	std::vector<unsigned long long> in;
	std::vector<double> knl;

	in.clear(); in.push_back(0);in.push_back(0);in.push_back(0);in.push_back(5);in.push_back(0);in.push_back(0);in.push_back(0);
	knl.clear(); knl.push_back(0.8);knl.push_back(0.7);knl.push_back(0.6);knl.push_back(0.5);
	std::vector<double> out_A7_KNL4_full = Conv1D(in,knl,0);
	std::vector<double> out_A7_KNL4_same = Conv1D(in,knl,1);
	std::vector<double> out_A7_KNL4_valid= Conv1D(in,knl,2);

	in.clear(); in.push_back(0);in.push_back(0);in.push_back(0);in.push_back(5);in.push_back(0);in.push_back(0);in.push_back(0);
	knl.clear(); knl.push_back(0.9);knl.push_back(0.8);knl.push_back(0.7);knl.push_back(0.6);knl.push_back(0.5);
	std::vector<double> out_A7_KNL5_full = Conv1D(in,knl,0);
	std::vector<double> out_A7_KNL5_same = Conv1D(in,knl,1);
	std::vector<double> out_A7_KNL5_valid= Conv1D(in,knl,2);

	in.clear(); in.push_back(0);in.push_back(0);in.push_back(0);in.push_back(5);in.push_back(5);in.push_back(0);in.push_back(0);in.push_back(0);
	knl.clear(); knl.push_back(0.8);knl.push_back(0.7);knl.push_back(0.6);knl.push_back(0.5);
	std::vector<double> out_A8_KNL4_full = Conv1D(in,knl,0);
	std::vector<double> out_A8_KNL4_same = Conv1D(in,knl,1);
	std::vector<double> out_A8_KNL4_valid= Conv1D(in,knl,2);

	in.clear(); in.push_back(0);in.push_back(0);in.push_back(0);in.push_back(5);in.push_back(5);in.push_back(0);in.push_back(0);in.push_back(0);
	knl.clear(); knl.push_back(0.9);knl.push_back(0.8);knl.push_back(0.7);knl.push_back(0.6);knl.push_back(0.5);
	std::vector<double> out_A8_KNL5_full = Conv1D(in,knl,0);
	std::vector<double> out_A8_KNL5_same = Conv1D(in,knl,1);
	std::vector<double> out_A8_KNL5_valid= Conv1D(in,knl,2);

	int test=0;
}
*/
