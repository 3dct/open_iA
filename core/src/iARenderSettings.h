#pragma once

class iARenderSettings
{
public:
	bool ShowVolume,
		ShowSlicers,
		ShowHelpers,
		ShowRPosition,
		ShowBoundingBox,
		ParallelProjection;
	double SampleDistance;
	QString BackgroundTop,
		BackgroundBottom;

	iARenderSettings():
		ShowVolume(true),
		ShowSlicers(false),
		ShowHelpers(true),
		ShowRPosition(true),
		ShowBoundingBox(true),
		ParallelProjection(false),
		SampleDistance(1),
		BackgroundTop("#7FAAFF"),
		BackgroundBottom("#FFFFFF")
	{}
};