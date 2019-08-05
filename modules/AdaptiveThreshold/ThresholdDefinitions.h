#pragma once
#include <vector>


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

