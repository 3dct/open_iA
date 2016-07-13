#pragma once

class iASingleSlicerSettings
{
public:
	bool LinearInterpolation,
		ShowIsoLines,
		ShowPosition;
	double MinIsoValue, MaxIsoValue;
	int NumberOfIsoLines;
	
	iASingleSlicerSettings() :
		LinearInterpolation(false),
		ShowIsoLines(false),
		ShowPosition(false),
		MinIsoValue(0),
		MaxIsoValue(0),
		NumberOfIsoLines(0)
	{}
};

class iASlicerSettings
{
public:
	bool InteractorsEnabled,
		LinkViews,
		LinkMDIs;
	int SnakeSlices;
	iASingleSlicerSettings SingleSlicer;

	iASlicerSettings() :
		InteractorsEnabled(true),
		LinkViews(false),
		LinkMDIs(false),
		SnakeSlices(0)
	{}

};