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

/** Class to create and handle image histogram, supports float32 and uint16 */
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
	int CreateHist(float* fImage, unsigned int nPixelH, unsigned int nPixelW, unsigned int nPixelD, int n, float min, float max, bool ConsiderRecoFillZeroes, unsigned long long nCutL, unsigned long long nCutH);
	unsigned int DetectPeaksValleys(unsigned int nPeaks, unsigned int dgauss_size_BINscale, unsigned int gauss_size_P2Pscale, double threshold_x, double threshold_y, bool consTruncBin);
	float CalcQ(std::vector<int> thrsh_IDX, std::vector<ClassMeasure> &result, int Q_equation);
	float CalcEntropy();

	std::vector<int> GetValleyThreshold_IDX();
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