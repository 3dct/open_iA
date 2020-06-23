#pragma once

//Qt
#include "qstring.h"

//vtk
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <vector>

struct iACompVisOptions 
{
	static const unsigned char BACKGROUNDCOLOR_GREY[3];
	static const unsigned char BACKGROUNDCOLOR_WHITE[3];
	
	static const unsigned char FONTCOLOR_TITLE[3];
	static const int FONTSIZE_TITLE;
	
	static const unsigned char FONTCOLOR_TEXT[3];
	static const int FONTSIZE_TEXT;

	//get an array filled with colors
	static unsigned char* getColorArray(double colors[3]);
	static double* getDoubleArray(const unsigned char colors[3]);
	

	static double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);

};