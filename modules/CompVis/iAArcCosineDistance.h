// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAProximityDistance.h"

/*
Formula implemented according to: " Lopes, A. M.; Machado, J. A. T. & Galhano, A. M.
Computational Comparison and Visualization of Viruses in the Perspective of Clinical Information
Interdisciplinary Sciences: Computational Life Sciences, Springer Science and Business Media LLC, 2017, 11, 86-94. "
*/
class iAArcCosineDistance : public iAProximityDistance
{
public:
	iAArcCosineDistance(std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems);
	virtual csvDataType::ArrayType* calculateProximityDistance();

private:
	double calculateCounter(std::vector<double> const& e1, std::vector<double> const& e2);
	double calculateDenominator(std::vector<double> const& e1, std::vector<double> const& e2);

};
