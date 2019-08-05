#pragma once
namespace algorithm {
	static bool greaterThan(double u, double v) {
		return u > v;
	}

	static bool smallerThan(double u, double v) {
		return u < v;
	}

	const double epsilon = 0.0000000001;

	static bool compareDouble(double a, double b) {
		return fabs(a - b) < epsilon;
	}

}