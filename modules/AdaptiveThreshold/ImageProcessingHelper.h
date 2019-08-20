#pragma once
#include "mdichild.h"
class ImageProcessingHelper
{
public: 
	ImageProcessingHelper(MdiChild* child) :m_childData(child)
	{
	}
		
	void performSegmentation(double greyThreshold);


private:
	ImageProcessingHelper(const ImageProcessingHelper& other) = delete; 
	ImageProcessingHelper() = delete; 
	MdiChild* m_childData = nullptr;

};

