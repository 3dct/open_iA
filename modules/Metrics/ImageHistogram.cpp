/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "ImageHistogram.h"

#include <vtkMath.h> // for vtkMath::Pi()

#include <algorithm>
#include <cmath>

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
		return log(n)*LOG2M1;
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

/** \fn int cImageHistogram::CreateHist(float* fImage, unsigned int nPixelH, unsigned int nPixelW, unsigned int nPixelD, int n, float min, float max, bool ConsiderRecoFillZeroes, unsigned long long nCutL, unsigned long long nCutH)
 *	\brief Calculate the histogram of the float image with n bins in the range of min to max. If truncation values >0, the initial histogramm is used to calculate a new min/max range. The final histogramm is calculated using the new truncated min/max range. If (nCutL+nCutH) is greater or equal datasize the truncation is disabled.
 *	\param fImage a float pointer to the source image.
 *	\param nPixelH a unsigned int. The image height in pixel.
 *	\param nPixelW a unsigned int. The image width in pixel.
 *	\param nPixelD a unsigned int. The image depth in pixel.
 *	\param n a int. The histogram has n bins, whose values are ranging from min...max in steps of (max-min)/(n-1).
 *	\param min a float.
 *	\param max a float.
 *	\param ConsiderRecoFillZeroes a bool. To consider reco fill zeroes or not. If not, the zero peak is replace with the mean of its neighbours
 *	\param nCutL a unsigned long long. The number of data points truncated from the lower value end.
 *	\param nCutH a unsigned long long. The number of data points truncated from the upper value end.
 *	\return a int. The number of used bins.
 */
int cImageHistogram::CreateHist(float* fImage, unsigned int nPixelH, unsigned int nPixelW, unsigned int nPixelD, int n, float min, float max, bool ConsiderRecoFillZeroes, unsigned long long nCutL, unsigned long long nCutH)
{
	bins = n;
	unsigned long long datasize = (unsigned long long)nPixelH*nPixelW*nPixelD;
	float curX, StepPerBin;
	std::vector<unsigned long long> trunc_idx; trunc_idx.push_back(0); trunc_idx.push_back(n-1);

	// Two iterations are only needed if several counts should be cuted from beginning and end of the first iterations histogram
	int runs=1;
	if( ((nCutL>0) || (nCutH>0)) && (nCutL+nCutH)<datasize)
		runs=2;
	for(int r=0; r<runs; r++)
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
		int zero_idx = (int)floor(((0.0f-min)/StepPerBin)+0.5f);

		// Calculate histogram by indexing
		for(unsigned long long index=0; index<datasize; index++)
		{
			curX = floor(((fImage[index]-min)/StepPerBin)+0.5f);
			if(curX<0)
				hist_y[0]++;
			else if (curX>n-1)
				hist_y[n-1]++;
			else
				hist_y[(int)curX]++;
		}

		// Calculate new min/max values with truncation
		if(r==0 && runs==2)
		{
			// Accumulate data to the lower truncation index
			unsigned long long sum=0;
			for(unsigned long long t=0; t<(unsigned int)n; t++)
			{
				sum += hist_y[t];
				if(sum > nCutL)
				{
					if(t>0)
						trunc_idx[0] = t-1;

					break;
				}
			}

			// Accumulate data to the upper truncation index
			sum=0;
			for(unsigned long long t=(unsigned int)n-1; t>=0; t--)
			{
				sum += hist_y[t];
				if(sum > nCutH)
				{
					if(t<n-1)
						trunc_idx[1] = t+1;

					break;
				}
			}
		}
		else
		{
			// Handle reco fill zeroes
			if(!ConsiderRecoFillZeroes && zero_idx==0)
				hist_y[zero_idx] = hist_y[1];
			else if(!ConsiderRecoFillZeroes && zero_idx==bins-1)
				hist_y[zero_idx] = hist_y[bins-2];
			else if(!ConsiderRecoFillZeroes && zero_idx>0 && zero_idx<bins-1)
				hist_y[zero_idx] = (unsigned long long)(((double)hist_y[zero_idx-1]+hist_y[zero_idx+1])*0.5+0.5);
		}
	}

	return (int)hist_y.size();
}


/** \fn unsigned int cImageHistogram::DetectPeaksValleys(unsigned int nPeaks, unsigned int dgauss_size_BINscale, unsigned int gauss_size_P2Pscale, double threshold_x , double threshold_y, bool consTruncBin);
 *	\brief The algorithm searches nPeaks peaks and nPeaks-1 valleys. x-positions and heights per peak and valley are save in member variables. The highest nPeaks peaks found by y'=0 and y''=neg are detected. nPeaks-1 minima are calculated between two peaks by finding the minimal value after gaussian smoothing in the histogram (Gauss kernel size 1/gauss_size_P2Pscale of peak distance). If consTruncBin true: the first and last bin are considered as possible, setting to false may be necessary if the histogram range has been truncated for the generation of the histogram.
*	\param nPeaks a unsigned int. The number of peaks to find
 *	\param gauss_size_P2Pscale a unsigned int. Gauss smoothing for finding local minima between two peaks, Gauss kernel size is (peak to peak distance)/gauss_sz_scal, typical value 20, the kernel size will be forced to odd values, setting the vallue to histbins will end in a size of 3
 *	\param dgauss_size_BINscale a unsigned int. Derivated gauss kernel size is histbins/dgauss_size_BINscale, typical values are 128,64,32 for histbins=512, the kernel size will be forced to odd values, setting the vallue to histbins will end in a size of 3
 *	\param threshold_x a double. Peaks are vaild if x is equal or greater this threshold.
 *	\param threshold_y a double. Peaks are vaild if y is equal or greater this threshold.
 *	\param consTruncBin a bool. If consTruncBin true: the first and last bin are considered as possible, setting to false may be necessary if the histogram range has been truncated for the generation of the histogram.
 *	\return a unsigned int. The number of found peaks.
 */
unsigned int cImageHistogram::DetectPeaksValleys(unsigned int nPeaks, unsigned int dgauss_size_BINscale, unsigned int gauss_size_P2Pscale, double threshold_x , double threshold_y, bool consTruncBin)
{
	std::vector<double> gauss_knl, dgauss_knl;

	// Generate Gauss kernel
	// =========================================================================
	int dknl_sz = (int)floor((0.5f*bins/dgauss_size_BINscale+0.5f));
	if(dknl_sz<1)
		dknl_sz = 1;                 // y' via gauss': [1.184 0 -1.184] ~= diff: [1 -1]
	float gauss_sigma = (float)dknl_sz/4;

	double sum=0.0;
	for(int i=-dknl_sz; i<=dknl_sz; i++)
	{
		double value=1/(std::sqrt(2*vtkMath::Pi())*gauss_sigma)*std::exp(-0.5*i*i/(gauss_sigma*gauss_sigma));
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
			int knl_sz = (int)floor(0.5f*abs((float)Peaks[p+1].idx-Peaks[p].idx)/gauss_size_P2Pscale+0.5f);
			if(knl_sz<1)
				knl_sz = 1;                 // y' via gauss': [1.184 0 -1.184] ~= diff: [1 -1]
			float new_gauss_sigma = (float)knl_sz/4;

			sum = 0.0;
			for(int i=-knl_sz; i<=knl_sz; i++)
			{
				double value = 1/(sqrt(2*vtkMath::Pi())* new_gauss_sigma)*exp(-0.5*i*i/(new_gauss_sigma * new_gauss_sigma));
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

/** \fn float cImageHistogram::CalcQ(std::vector<int> thrsh_IDX, std::vector<ClassMeasure> &result, int Q_equation)
 *	\brief The algorithm calculates quality measures in total and per class of the histogram using the given thresholds.
 *	\param thrsh_IDX a std::vector<int>. The indices of the thresholds.
 *	\param result a std::vector<ClassMeasure>. The quality measures per class.
 *	\param Q_equation a int. Select the used Q equation. 0 ... sqrt(sigma*sigma) else ... sqrt(sigma^2+sigma^2)
 *	\return a float. The number total quality measure.
 */
 float cImageHistogram::CalcQ(std::vector<int> thrsh_IDX, std::vector<ClassMeasure> &result, int Q_equation)
{
	int CStart,CEnd,
		maxprobabilityClassIDX = -1;
	unsigned long long nval;
	double sum,count,counttotal=0,maxprobability=0.0;
	ClassMeasure val;
	result.clear();

	// Total count of data points
	for(std::vector<unsigned long long>::iterator iter = hist_y.begin(); iter < hist_y.end(); iter++)
		counttotal += *iter;

	// Sort by increasing indices
	std::sort(thrsh_IDX.begin(),thrsh_IDX.end());

	// Calculate data per class, divided by the thresholds
	for(int t=0; t<=(int)thrsh_IDX.size(); t++)
	{
		if(t==0)
		{
			CStart = 0;
			CEnd = thrsh_IDX[t];
		}
		else if(t==thrsh_IDX.size())
		{
			CStart = thrsh_IDX[t-1];
			CEnd = (int)hist_y.size();
		}
		else
		{
			CStart = thrsh_IDX[t-1];
			CEnd = thrsh_IDX[t];
		}

		// mean
		count=0,sum=0,nval=0;
		for(int i=CStart; i<CEnd; i++)
		{
			count+=hist_y[i];
			sum+=static_cast<double>(hist_y[i])*hist_x[i];
			nval+=hist_y[i];
		}
		val.mean=(float)(sum/nval);

		// standard deviation
		sum=0;
		for(int i=CStart; i<CEnd; i++)
			sum += pow((double)hist_x[i]-val.mean,2)*hist_y[i];

		val.sigma=(float)pow(sum/nval,0.5);

		//probability
		val.probability = (float)(count/counttotal);

		if(t>0 && val.probability>maxprobability)
		{
			maxprobability = val.probability;
			maxprobabilityClassIDX = t;
		}
		val.UsedForQ=0;
		val.LowerThreshold = hist_x[CStart];
		val.UpperThreshold = hist_x[CEnd-1];
		result.push_back(val);
	}
	result[0].UsedForQ = 1;
	result[maxprobabilityClassIDX].UsedForQ = 2;

	// Total quality measure between the lowest absorbing (air) and the peak with highest probability
	if(Q_equation==0)
		return abs(result[maxprobabilityClassIDX].mean-result[0].mean)/std::sqrt(result[maxprobabilityClassIDX].sigma*result[0].sigma);
	else
		return abs(result[maxprobabilityClassIDX].mean-result[0].mean)/std::sqrt(result[maxprobabilityClassIDX].sigma*result[maxprobabilityClassIDX].sigma+result[0].sigma*result[0].sigma);
}

 /** \fn float cImageHistogram::CalcEntropy(std::vector<int> , std::vector<ClassMeasure> &result, int Q_equation)
 *	\brief The algorithm calculates the relative Shannon Entropy measure
 *	\return a float. The Entropy.
 */
float cImageHistogram::CalcEntropy()
{
	double H=0.0,Hmax=0.0,counttotal=0;

	// Total count of data points
	for(std::vector<unsigned long long>::iterator iter = hist_y.begin(); iter < hist_y.end(); iter++)
		counttotal += *iter;

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
		if(iter->y>=DoubleEpsilon)
			H+=(iter->y*dlog2(iter->y));
	}
	H/=-Hmax;

	return (float)H;
}

/** \fn std::vector<int> cImageHistogram::GetValleyThreshold_IDX()
 *	\brief Return the indices of the precalculated valley thresholds.
 *	\return a std::vector<int>. The calculated thresholds.
 */
std::vector<int> cImageHistogram::GetValleyThreshold_IDX()
{
	std::vector<int> thrsh;

	for(HistPosList::iterator iter = Valleys.begin(); iter < Valleys.end(); iter++)
		thrsh.push_back(iter->idx);

	return thrsh;
}

/** \fn std::vector<float> cImageHistogram::GetValleyThreshold()
 *	\brief Return the precalculated valley thresholds.
 *	\return a std::vector<float>. The calculated thresholds.
 */
std::vector<float> cImageHistogram::GetValleyThreshold()
{
	std::vector<float> thrsh;

	for(HistPosList::iterator iter = Valleys.begin(); iter < Valleys.end(); iter++)
		thrsh.push_back(iter->x);

	return thrsh;
}

std::vector<double> cImageHistogram::Conv1D(std::vector<unsigned long long> in, std::vector<double> knl, int mode)
{
	std::vector<double> out;
	int i, iext, k, kext, extsize = ((int)knl.size()-1), fullsize = (int)in.size()+(int)knl.size()-1, hlpsize;
	if(knl.size()%2)
		hlpsize = ((int)knl.size()-1)/2;
	else
		hlpsize = (int)knl.size()/2-1;

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
		for(k=(int)knl.size()-1, kext=-hlpsize; k>=0; k--, kext++)
			out[i] += knl[k] * in[iext+kext];
	}

	if(mode==1)
	{
		// Reduce to same (core) size
		if(knl.size()%2)
			out.erase(out.begin(),out.begin()+hlpsize);
		else
			out.erase(out.begin(),out.begin()+hlpsize+1);
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
