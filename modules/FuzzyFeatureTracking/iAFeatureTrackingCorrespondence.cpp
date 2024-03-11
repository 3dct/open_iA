// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureTrackingCorrespondence.h"

iAFeatureTrackingCorrespondence::iAFeatureTrackingCorrespondence(vtkIdType id, float overlap, float volumeRatio,
	bool isTakenForCurrentIteration, float likelyhood, FeatureEvent featureEvent) {
	this->id = id;
	this->overlap = overlap;
	this->volumeRatio = volumeRatio;
	this->isTakenForCurrentIteration = isTakenForCurrentIteration;
	this->likelyhood = likelyhood;
	this->featureEvent = featureEvent;
}
