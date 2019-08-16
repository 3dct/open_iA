#pragma once
#include <vector>
#include <QString>
#include <stdexcept>
#include <QPointF>

namespace threshold_defs {

	const double dblInf_min = -std::numeric_limits<double>::infinity();
	const double fltInf_min = -std::numeric_limits<float>::infinity();
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
			FreqPeakMinY(dblInf_min);
			PeakMinXThreshold(dblInf_min);
			PeakHalf(dblInf_min);

			Iso50ValueThr(dblInf_min);
				/*double iso50ValueFreq; */

			FreqPeakLokalMaxY(dblInf_min);
			LokalMaxPeakThreshold_X(dblInf_min);

			IntersectionPoint = QPointF(fltInf_min,fltInf_min);
		}


		QPointF createLokalMaxHalfPoint() {
			return QPointF((float)LokalMaxPeakThreshold_X(), (float)PeakHalf());
		}


		QString toString() {
			QString res = QString("Min %1 \t %2 Max %3 \t %4").arg(PeakMinXThreshold()).arg(FreqPeakMinY()).arg(LokalMaxPeakThreshold_X()).arg(FreqPeakLokalMaxY());
			return res;

		}


		
		double LokalMaxPeakThreshold_X() const { return lokalMaxPeakThreshold_X; }
		void LokalMaxPeakThreshold_X(double val) { lokalMaxPeakThreshold_X = val; }
	public:

		double FreqPeakMinY() const { return freqPeakMinY; }
		void FreqPeakMinY(double val) { freqPeakMinY = val; }
		double PeakMinXThreshold() const { return peakMinXThreshold; }
		void PeakMinXThreshold(double val) { peakMinXThreshold = val; }
		double PeakHalf() const { return fPeakHalf; }
		void PeakHalf(double val) { fPeakHalf = val; }
		double Iso50ValueThr() const { return iso50ValueThr; }
		void Iso50ValueThr(double val) { iso50ValueThr = val; }
		double FreqPeakLokalMaxY() const { return freqPeakLokalMaxY; }
		void FreqPeakLokalMaxY(double val) { freqPeakLokalMaxY = val; }
		void setIntersectionPoint(const QPointF& pt) {
			IntersectionPoint = pt; 
		}

		const QPointF& getIntersectionPoint(){
			return IntersectionPoint; 
		}


	private:
		double freqPeakMinY;
		double peakMinXThreshold;
		double fPeakHalf; 

		double iso50ValueThr; 
		/*double iso50ValueFreq; */

		double freqPeakLokalMaxY;
		double lokalMaxPeakThreshold_X;

		QPointF IntersectionPoint;

		
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