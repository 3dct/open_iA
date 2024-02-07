// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "featurescout_export.h"

#include <iAGUIModuleInterface.h>

struct iACsvConfig;

class FeatureScout_API iAFeatureScoutModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private slots:
	void featureScout();
};
