//#include "iACompVisOptions.h"

//Debug
//#include "iALog.h"
//
//const unsigned char iACompVisOptions::BACKGROUNDCOLOR_GREY[3] = { 25, 25, 25 };//{128, 128, 128 };
//const unsigned char iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY[3] = { 115, 115, 115 };
//const unsigned char iACompVisOptions::BACKGROUNDCOLOR_LIGHTERGREY[3] = { 189, 189, 189 };
//const unsigned char iACompVisOptions::BACKGROUNDCOLOR_WHITE[3] = { 255, 255, 255};
//
//const unsigned char iACompVisOptions::HIGHLIGHTCOLOR_BLACK[3] = { 0, 0, 0 };
//const unsigned char iACompVisOptions::HIGHLIGHTCOLOR_YELLOW[3] = { 255,237,160 };
//const unsigned char iACompVisOptions::HIGHLIGHTCOLOR_GREEN[3] = { 31,179,81};
//
//
//const unsigned char iACompVisOptions::FONTCOLOR_TITLE[3] = { 239, 239, 239 };//{ 195, 195, 195 };//{255, 255, 255};
//const int iACompVisOptions::FONTSIZE_TITLE = 20;
//
//const unsigned char iACompVisOptions::FONTCOLOR_TEXT[3] = { 239, 239, 239 };//{ 255, 255, 255 };
//const int iACompVisOptions::FONTSIZE_TEXT = 13;
//
//const int iACompVisOptions::LINE_WIDTH = 5; //3
//
//unsigned char* iACompVisOptions::getColorArray(double colors[3])
//{
//	unsigned char result[3];
//
//	for (size_t j = 0; j < 3; ++j)
//	{
//		result[j] = static_cast<unsigned char>(colors[j] * 255);
//	}
//
//	return result;
//}
//
//double* iACompVisOptions::getDoubleArray(const unsigned char colors[3])
//{
//	double result[3];
//	double help[3];
//	help[0] = static_cast<double>(colors[0]);
//	help[1] = static_cast<double>(colors[1]);
//	help[2] = static_cast<double>(colors[2]);
//
//	result[0] = histogramNormalization(help[0], 0.0, 1.0, 0.0, 255);
//	result[1] = histogramNormalization(help[1], 0.0, 1.0, 0.0, 255);
//	result[2] = histogramNormalization(help[2], 0.0, 1.0, 0.0, 255);
//
//	return result;
//}
//
//QColor iACompVisOptions::getQColor(const unsigned char colors[3])
//{
//	int r = colors[0];
//	int g = colors[1];
//	int b = colors[2];
//	QColor result = QColor(r,g,b);
//
//	return result;
//}
//
//double iACompVisOptions::histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax)
//{
//	double result = ((newMax - newMin)* ((value - oldMin) / (oldMax - oldMin))) + newMin;
//	return result;
//}
//
