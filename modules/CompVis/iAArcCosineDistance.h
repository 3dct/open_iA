/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
