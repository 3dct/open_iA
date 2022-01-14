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
