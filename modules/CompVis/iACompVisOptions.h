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

#include <vtkSmartPointer.h>

#include <QColor>

#include <vector>

class vtkActor;

namespace iACompVisOptions
{
	const unsigned char BACKGROUNDCOLOR_GREY[3] = { 25, 25, 25 };//{128, 128, 128 };
	const unsigned char BACKGROUNDCOLOR_LIGHTGREY[3] = { 115, 115, 115 };
	const unsigned char BACKGROUNDCOLOR_LIGHTERGREY[3] = { 189, 189, 189 };
	const unsigned char BACKGROUNDCOLOR_WHITE[3] = { 255, 255, 255 };

	const unsigned char HIGHLIGHTCOLOR_BLACK[3] = { 0, 0, 0 };
	const unsigned char HIGHLIGHTCOLOR_YELLOW[3] = { 255,237,160 };
	const unsigned char HIGHLIGHTCOLOR_GREEN[3] = { 31,179,81 };

	const unsigned char FONTCOLOR_TITLE[3] = { 239, 239, 239 };//{ 195, 195, 195 };//{255, 255, 255};
	const int FONTSIZE_TITLE = 20;

	const unsigned char FONTCOLOR_TEXT[3] = { 239, 239, 239 };//{ 255, 255, 255 };
	const int FONTSIZE_TEXT = 13;

	const int LINE_WIDTH = 5; //3

	void getColorArray(double colors[3], unsigned char result[3]);

	QColor getQColor(const unsigned char colors[3]);

	double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);

	void getDoubleArray(const unsigned char colors[3], double result[3]);

	double round_up(double value, int decimal_places);

	std::string cutStringAfterNDecimal(std::string input, int decimal_places);

	template <typename T>
	void copyVector(std::vector<T> const * toCopy, std::vector<T>* copied)
	{
		std::copy(toCopy->begin(), toCopy->end(), copied->begin());
	}

	void stippledLine(vtkSmartPointer<vtkActor> actor, int lineStipplePattern, int lineStippleRepeat);
};
