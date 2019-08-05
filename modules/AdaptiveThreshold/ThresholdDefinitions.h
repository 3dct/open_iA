#pragma once
#include <vector>
#include <QString>

namespace threshold_defs {

	//Ranges in XY direction
	class ParametersRanges {

	public:
		ParametersRanges() {
			x_vals.reserve(1000);
			y_vals.reserve(1000);
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

		double minThreshold;
		double x_value;

		double maxThreshold;
		double y_value;
	};


}; 