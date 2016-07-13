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
	
	iARenderSettings(bool showVolume,
			bool showSlicers,
			bool showHelpers,
			bool showRPosition,
			bool showBoundingBox,
			bool parallelProjection,
			double sampleDistance,
			QString backgroundTop,
			QString backgroundBottom):
		ShowVolume        (showVolume),
		ShowSlicers       (showSlicers),
		ShowHelpers       (showHelpers),
		ShowRPosition     (showRPosition),
		ShowBoundingBox   (showBoundingBox),
		ParallelProjection(parallelProjection),
		SampleDistance    (sampleDistance),
		BackgroundTop     (backgroundTop),
		BackgroundBottom  (backgroundBottom)
	{}
};