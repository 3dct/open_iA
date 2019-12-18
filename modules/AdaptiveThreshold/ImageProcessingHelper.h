#pragma once
#include "mdichild.h"
class ImageProcessingHelper
{
public: 
	ImageProcessingHelper(MdiChild* child) :m_childData(child)
	{
	}
		
	void performSegmentation(double greyThresholdMin, double greyThreshold);

	void prepareFilter(double greyThresholdLower, double greyThresholdUpper);

	
private:
	void imageToReslicer();

	ImageProcessingHelper(const ImageProcessingHelper& other) = delete; 
	ImageProcessingHelper() = delete; 
	MdiChild* m_childData = nullptr;

};

