// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkType.h>

enum FeatureEvent
{
	Creation,
	Continuation,
	Bifurcation,   // = Split
	Amalgamation,  // = Merge
	Dissipation
};

class iAFeatureTrackingCorrespondence
{
public:
	iAFeatureTrackingCorrespondence(vtkIdType id, float overlap, float volumeRatio,
		bool isTakenForCurrentIteration, float likelyhood, FeatureEvent featureEvent);
	vtkIdType id;
	float overlap;
	float volumeRatio;
	bool isTakenForCurrentIteration;
	float likelyhood;
	FeatureEvent featureEvent;
};
