#include <iAQMeasureCalculation.h>
#include <ImageHistogram.h>
#include <format> 

std::map<std::string, double> iAQMeasureCalculation::computeOrigQ(
	float* fImage, const int* dim, const double* range, int HistogramBins, int NumberPeaks, bool AnalyzePeak = false)
{
	// some "magic numbers"
	unsigned int dgauss_size_BINscale = 24;
	unsigned int gauss_size_P2Pscale = 24;
	double threshold_x = -0.1;
	double threshold_y = 2;  // one single voxel is no valid class


	//float* fImage = static_cast < float*>(img->GetScalarPointer());
	cImageHistogram curHist;
	curHist.CreateHist(fImage, dim[0], dim[1], dim[2], HistogramBins, range[0], range[1], false, 0, 0);
	/*unsigned int Peaks_fnd = */ curHist.DetectPeaksValleys(
		NumberPeaks, dgauss_size_BINscale, gauss_size_P2Pscale, threshold_x, threshold_y, false);

	// Calculate histogram quality measures Q using the valley thresholds to separate classes
	std::vector<int> thresholds_IDX = curHist.GetValleyThreshold_IDX();
	std::vector<float> thresholds = curHist.GetValleyThreshold();
	std::vector<ClassMeasure> classMeasures;
	double Q0 = (thresholds_IDX.size() == 0) ? 0.0 : curHist.CalcQ(thresholds_IDX, classMeasures, 0);
	double Q1 = (thresholds_IDX.size() == 0) ? 0.0 : curHist.CalcQ(thresholds_IDX, classMeasures, 1);
	
	
	std::map<std::string, double> results;

	results.insert({"Q (orig, equ 0)", Q0});
	results.insert({"Q (orig, equ 1)", Q1});
	if (AnalyzePeak)
	{

		int classNr = 0;
		for (auto c : classMeasures)
		{
			results.insert({std::format("Peak {} Mean", classNr), c.mean});
			results.insert({std::format("Peak {} Sigma",classNr), c.sigma});
			results.insert({std::format("Peak {} Probability",classNr), c.probability});
			results.insert({std::format("Peak {} Min",classNr), c.LowerThreshold});
			results.insert({std::format("Peak {} Max",classNr), c.UpperThreshold});
			results.insert({std::format("Peak {} Usage",classNr), c.UsedForQ});
			++classNr;
		}
	}
	

	return results;
}