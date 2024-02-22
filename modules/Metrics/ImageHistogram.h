// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vector>

struct ClassMeasure
{
	float mean;				//!< mean greyvalue of the class
	float sigma;			//!< standard deviation of the mean greyvalue of the class
	float probability;		//!< occurence probability of the class
	unsigned char UsedForQ;	//!< Class used for the calculation of Q, 0...unused class, 1...selected as air class, 2...selected as material class
	float LowerThreshold;	//!< Lower threshold to the previous class
	float UpperThreshold;	//!< Upper threshold to the next class
};

//! Class to create and handle image histogram, supports float32 and uint16
class cImageHistogram
{
public:

	//! Struct for histogram positions with double values
	struct dHistPos
	{
		int idx;	//!< index of the histogram position
		double y;	//!< y value of the histogram position
	};

	//! Struct for histogram positions like peaks, valley and tresholds
	struct HistPos
	{
		int idx;				//!< index of the histogram position
		float x;				//!< x value of the histogram position
		unsigned long long	y;	//!< y value of the histogram position
	};
	//!	Calculate the histogram of the float image with n bins in the range of min to max.
	//! If truncation values >0, the initial histogramm is used to calculate a new min/max range.
	//! The final histogramm is calculated using the new truncated min/max range.
	//! If (nCutL+nCutH) is greater or equal datasize the truncation is disabled.
	//!	@param fImage a float pointer to the source image.
	//!	@param nPixelH a unsigned int. The image height in pixel.
	//!	@param nPixelW a unsigned int. The image width in pixel.
	//!	@param nPixelD a unsigned int. The image depth in pixel.
	//!	@param n a int. The histogram has n bins, whose values are ranging from min...max in steps of (max-min)/(n-1).
	//!	@param min a float.
	//!	@param max a float.
	//!	@param ConsiderRecoFillZeroes a bool. To consider reco fill zeroes or not. If not, the zero peak is replace with the mean of its neighbours
	//!	@param nCutL a unsigned long long. The number of data points truncated from the lower value end.
	//!	@param nCutH a unsigned long long. The number of data points truncated from the upper value end.
	//!	@return the number of used bins.
	int CreateHist(float* fImage, unsigned int nPixelH, unsigned int nPixelW, unsigned int nPixelD, int n, float min, float max, bool ConsiderRecoFillZeroes, unsigned long long nCutL, unsigned long long nCutH);

	//! Searches peaks and valleys.
	//! Searches nPeaks peaks and nPeaks-1 valleys, x-positions and heights per peak and valley are saved in member variables.
	//! The highest nPeaks peaks found by y'=0 and y''=neg are detected.
	//! nPeaks-1 minima are calculated between two peaks by finding the minimal value after gaussian smoothing in the histogram
	//! (Gauss kernel size 1/gauss_size_P2Pscale of peak distance).
	//! If consTruncBin true: the first and last bin are considered as possible,
	//! setting to false may be necessary if the histogram range has been truncated for the generation of the histogram.
	//! @param nPeaks a unsigned int. The number of peaks to find
	//! @param gauss_size_P2Pscale a unsigned int. Gauss smoothing for finding local minima between two peaks,
	//!        Gauss kernel size is (peak to peak distance)/gauss_sz_scal, typical value 20, the kernel size
	//!        will be forced to odd values, setting the vallue to histbins will end in a size of 3
	//! @param dgauss_size_BINscale a unsigned int. Derivated gauss kernel size is histbins/dgauss_size_BINscale,
	//!        typical values are 128,64,32 for histbins=512, the kernel size will be forced to odd values,
	//!        setting the vallue to histbins will end in a size of 3
	//! @param threshold_x a double. Peaks are vaild if x is equal or greater this threshold.
	//! @param threshold_y a double. Peaks are vaild if y is equal or greater this threshold.
	//! @param consTruncBin a bool. If consTruncBin true: the first and last bin are considered as possible,
	//!        setting to false may be necessary if the histogram range has been truncated for the generation of the histogram.
	//! @return the number of found peaks.
	unsigned int DetectPeaksValleys(unsigned int nPeaks, unsigned int dgauss_size_BINscale, unsigned int gauss_size_P2Pscale, double threshold_x, double threshold_y, bool consTruncBin);

	//! Calculates quality measures in total and per class of the histogram using the given thresholds.
	//! @param thrsh_IDX a std::vector<int>. The indices of the thresholds.
	//! @param result a std::vector<ClassMeasure>. The quality measures per class.
	//! @param Q_equation a int. Select the used Q equation. 0 ... sqrt(sigma*sigma) else ... sqrt(sigma^2+sigma^2)
	//! @return the total quality measure number.
	float CalcQ(std::vector<int> thrsh_IDX, std::vector<ClassMeasure> &result, int Q_equation);

	//! Calculates the relative Shannon Entropy measure.
	//! @return the Entropy.
	float CalcEntropy();

	//! Return the indices of the precalculated valley thresholds.
	//! @return the calculated thresholds.
	std::vector<int> GetValleyThreshold_IDX();

	//! Return the precalculated valley thresholds.
	//! @return the calculated thresholds.
	std::vector<float> GetValleyThreshold();
private:

	typedef std::vector<HistPos> HistPosList;		//!< List of histogram positions
	typedef std::vector<dHistPos> dHistPosList;		//!< List of histogram positions with double values
	std::vector<double> Conv1D(std::vector<unsigned long long> in, std::vector<double> knl, int mode);	//!< 1D convolution of in with knl. mode 0 returns the full convolution. mode 1 returns the central part of the convolution of the same size as u. mode 2 returns only those parts of the convolution that are computed without the zero-padded edges.

	std::vector<unsigned long long> hist_y;		//!< y-axes of the histogram
	std::vector<float>				hist_x;		//!< x-axes of the histogram
	std::vector<double>				ghist_y;	//!< y-axes of the derivated smoothed histogram
	std::vector<float>				ghist_x;	//!< x-axes of the derivated smoothed histogram
	std::vector<double>				dghist_y;	//!< y-axes of the derivated smoothed histogram
	std::vector<float>				dghist_x;	//!< x-axes of the derivated smoothed histogram

	HistPosList Peaks;							//!< Histogram peaks
	HistPosList Valleys;						//!< Histogram valleys

	int bins;									//!< number of bins
};
