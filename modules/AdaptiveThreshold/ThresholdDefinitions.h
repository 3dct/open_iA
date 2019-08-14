#pragma once
#include <vector>
#include <QString>
#include <stdexcept>
#include <QPointF>

namespace threshold_defs {

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

	struct ThresMinMax {

		double freqPeakMinY;
		double peakMinXThreshold;
		double fPeakHalf; 

		double iso50ValueThr; 
		/*double iso50ValueFreq; */

		double freqPeakLokalMaxY;
		double lokalMaxPeakThreshold_X;

		QPointF createLokalMaxHalfPoint(){
			return QPointF((float)lokalMaxPeakThreshold_X, (float)fPeakHalf);
		}


		QString toString() {
			QString res = QString("Min %1 \t %2 Max %3 \t %4").arg(peakMinXThreshold).arg(freqPeakMinY).arg(lokalMaxPeakThreshold_X).arg(freqPeakLokalMaxY);
			return res; 

		}
	};

	struct PeakRanges {
		double XRangeMIn;
		double XRangeMax; 

		double HighPeakXmin;
		double HighPeakXMax;
		
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