#pragma once
#include <vector>
#include <QString>
#include <stdexcept>
#include <QPointF>
#include "iAMathUtility.h"

namespace threshold_defs {

	const double dblInf_min = -std::numeric_limits<double>::infinity();
	const double fltInf_min = -std::numeric_limits<float>::infinity();

	//storing lokal and global maximum
	class GreyThresholdPeaks
	{
	public: 
		GreyThresholdPeaks():m_lokalMax(dblInf_min), m_globalMax(dblInf_min){}


	void init(double minThr, double maxThr)
	{
		m_lokalMax = minThr;
		m_globalMax = maxThr; 
	}

	double getLocalMax() const {
		return m_lokalMax; 
	}

	double getGlobalMax() const {

		return m_globalMax; 
	}


	private:

		double m_lokalMax; //<!Air peak
		double m_globalMax; 
	};

	//Ranges in XY direction
	class ParametersRanges {

	public:
		ParametersRanges() {
			x_vals.reserve(1000);
			y_vals.reserve(1000);
		}

		ParametersRanges(const std::vector<double> &xVals, const std::vector<double> &yVals)
			:x_vals(xVals), y_vals(yVals) {

		}

		void insertElem(double x, double y) {
			x_vals.push_back(x);
			y_vals.push_back(y);
		}


		void clearElemems() {
			x_vals.clear(); 
			y_vals.clear(); 
			x_vals.reserve(1000);
			y_vals.reserve(1000); 
		
		}

		const std::vector<double>& getXRange() const {
			return x_vals;
		}
		const std::vector<double>& getYRange() const {
			return y_vals;
		}


		QString toString() {
			QString res = "";
			size_t x_size = x_vals.size();

			for (size_t i = 0; i < x_size; ++i) {
				QString	 tmp = QString("Pair %1 %2\n").arg(x_vals[i]).arg(y_vals[i]);
				res += tmp;
			}
		}

		double getXMin() const {
			double min = *std::min_element(std::begin(x_vals), std::end(x_vals));
			return min; 
		};

		double getXMax()const {
			double xmax = *std::max_element(std::begin(x_vals), std::end(x_vals));
			return xmax; 
		}


		void setXVals(const std::vector<double> &vals) {
			x_vals = vals;
		}

		bool isXEmpty() const  { return x_vals.empty(); }
		bool isYEmpty() const  { return y_vals.empty();  }
		//friend class thresCalcHelper; 

	private:
		
		std::vector<double> x_vals;
		std::vector<double> y_vals;
	};

	//storing xInd and threshold
	struct ThresIndx {
		ThresIndx() {
			thrIndx = -std::numeric_limits<long int>::infinity();
			value = -std::numeric_limits<double>::infinity();
		}

		long int thrIndx;
		double value;

	};

	class ThresMinMax {
	public:
		ThresMinMax() 
		{
 			initValues();

		}

		void initValues()
		{
			FreqPeakMinY(dblInf_min);
			PeakMinXThreshold(dblInf_min);
			fAirPeakHalf(dblInf_min);

			Iso50ValueThr(dblInf_min);

			FreqPeakLokalMaxY(dblInf_min);
			LokalMaxPeakThreshold_X(dblInf_min);
			DeterminedThreshold(dblInf_min);
			//setSpecifiedMinMax(dblInf_min, dblInf_min); 

			IntersectionPoint = QPointF(fltInf_min, fltInf_min);
		}


		QPointF createLokalMaxHalfPoint() {
			return QPointF((float)LokalMaxPeakThreshold_X(), (float)fAirPeakHalf());
		}


		QString MinMaxToString() {
			QString res = QString("Min %1 \t %2 Max %3 \t %4").arg(PeakMinXThreshold()).arg(FreqPeakMinY()).arg(LokalMaxPeakThreshold_X()).arg(FreqPeakLokalMaxY());
			return res;

		}


		
		double LokalMaxPeakThreshold_X() const { return lokalPeakAirThrX; }
		void LokalMaxPeakThreshold_X(double val) { lokalPeakAirThrX = val; }
		double DeterminedThreshold() const { return determinedThreshold; }
		void DeterminedThreshold(double val) { determinedThreshold = val; }
	public:

		
		void normalizeXValues(double min, double max)
		{
			peakMinXThreshold = minMaxNormalize(min, max, peakMinXThreshold);
			iso50ValueThr = minMaxNormalize(min, max, iso50ValueThr); 
			lokalPeakAirThrX = minMaxNormalize(min, max, lokalPeakAirThrX);
			MaterialPeakThrX = minMaxNormalize(min, max, MaterialPeakThrX);
		}


		void mapNormalizedBackToMinMax(double min, double max) {

			peakMinXThreshold = normalizedToMinMax(min, max, peakMinXThreshold);
			iso50ValueThr = normalizedToMinMax(min, max, iso50ValueThr); 
			lokalPeakAirThrX = normalizedToMinMax(min, max, lokalPeakAirThrX);
			MaterialPeakThrX = normalizedToMinMax(min, max, MaterialPeakThrX);

		}
		
		//apply custom min max
		void updateMinMaxPeaks(double lokalMinX, double lokalMinY, double lokalMaxX, double lokalMaxY) {
			peakMinXThreshold = lokalMinX;
			freqPeakMinY = lokalMinY;
			freqPeakLokalMaxY = lokalMaxY;
			lokalPeakAirThrX = lokalMaxX;
			m_custimizedMinMax = true;

		}


		double FreqPeakMinY() const { return freqPeakMinY; }
		void FreqPeakMinY(double val) { freqPeakMinY = val; }
		double PeakMinXThreshold() const { return peakMinXThreshold; }
		void PeakMinXThreshold(double val) { peakMinXThreshold = val; }
		double fAirPeakHalf() const { return fPeakHalf; }
		void fAirPeakHalf(double val) { fPeakHalf = val; }
		double Iso50ValueThr() const { return iso50ValueThr; }
		void Iso50ValueThr(double val) { iso50ValueThr = val; }
		double FreqPeakLokalMaxY() const { return freqPeakLokalMaxY; }
		void FreqPeakLokalMaxY(double val) { freqPeakLokalMaxY = val; }
		void setIntersectionPoint(const QPointF& pt) {
			IntersectionPoint = pt; 
		}


		double getAirPeakThr() const  {
			return lokalPeakAirThrX;
		}


		double getMaterialsThreshold()  const {
			return MaterialPeakThrX;
		}

		void setMaterialsThreshold(double maxThr) {
			MaterialPeakThrX = maxThr;
		}

		const QPointF& getIntersectionPoint() const {
			return IntersectionPoint; 
		}

		bool custimizedMinMax() {
			return m_custimizedMinMax;
		}


		QString resultsToString(bool printFinalResult) {
			QString resSt = QString("Min peak %1  %2\n").arg(peakMinXThreshold).arg(freqPeakMinY);
			resSt += QString("Lokal Air Peak %1 %2\n").arg(lokalPeakAirThrX).arg(freqPeakLokalMaxY);
			resSt += QString("Intersection point %1 %2\n").arg(IntersectionPoint.x()).arg(IntersectionPoint.y());
			resSt += QString("iso 50 %1\n").arg(Iso50ValueThr()); 
			resSt += QString("Maximum Peak %1\n").arg(MaterialPeakThrX); 
			resSt += QString("Specified min max %1 %2").arg(specifiedMax).arg(specifiedMax); 

			if (printFinalResult)
				resSt += QString("final resulting grey value %1").arg(DeterminedThreshold()); 
			
			return resSt; 
		}
		


		//setting MinThreshold = Air Peak
		//Max = Material Peak
		/*void setSpecifiedMinMax(double min, double max) {
			specifiedMin = min; 
			specifiedMax = max; 
		}

		double getSpecifiedMin() const {
			return specifiedMin; 
		}

		double getSpecifiedMax() const {
			return specifiedMax; 
		}*/

		void setPeaksMinMax(double minThrPeakAir, double maxThrPeakMaterials) {

			m_peaks.init(minThrPeakAir, maxThrPeakMaterials); 

		}

	/*	double getLokalThrMin_Air() {
			m_peaks.getLocalMax();
		};
		double getLokalThrMax_Materials(){
			m_peaks.getGlobalMax(); 
		}*/

		GreyThresholdPeaks const& getGreyThresholdPeaks() const {
			return m_peaks; 

		}
		

	private:
		double freqPeakMinY;
		double peakMinXThreshold;
		double fPeakHalf; 

		double iso50ValueThr; //is this needed? 
		/*double iso50ValueFreq; */

		double freqPeakLokalMaxY;
		double lokalPeakAirThrX;

		double MaterialPeakThrX; //Material value, at global maximum 

		QPointF IntersectionPoint;


		double determinedThreshold; 

		bool m_custimizedMinMax = false; 

		//todo is is this needed? 
		double specifiedMax;
		double specifiedMin; 


		GreyThresholdPeaks m_peaks; 
		
	};

	

	struct PeakRanges {
		double XRangeMIn;
		double XRangeMax; 

		double HighPeakXmin;
		double HighPeakXMax;
		
	};

	class GraphRange {
	public: 
		GraphRange() {
			m_xmin = fltInf_min;
			m_xmax = fltInf_min;
			m_ymin = fltInf_min;
			m_ymax = fltInf_min; 
		}

		void initRange(double xmin, double xmax, double ymin, double ymax) {
			this->m_xmin = xmin;
			this->m_xmax = xmax; 
			this->m_ymin = ymin;
			this->m_ymax = ymax; 
		}

		const QString &toString() const {
			QString output = "Range x1:x2, y1:y2 ";
			return output += QString("%1, %2 %3, %4").arg(m_xmin).arg(m_xmax).arg(m_ymin).arg(m_ymax); 
		}

		double getYMin() { return m_ymin; }
		double getYMax() { return m_ymax; }
		double getXMax() { return m_xmax;  }
		double getxmin() { return m_xmin;  }


	private:
		double m_xmin; 
		double m_xmax; 
		double m_ymin; 
		double m_ymax; 
	};



	/*
	*Storing averages of a histogram
	*/
	class MovingFreqs {
	public:
		inline	void addSequence(std::vector<double>& values) {
			m_sequences.push_back(values);
		};

		const std::vector<double> &getFrequencyByInd(uint index) const {
			if (index >= m_sequences.size()) {
				throw std::invalid_argument("not in index list");
			}

			else return m_sequences.at(index);
		}

		inline const std::vector < std::vector<double>> getAll(){
			return m_sequences;
		}

	private:
		std::vector<std::vector<double>> m_sequences; 

	};


}; 