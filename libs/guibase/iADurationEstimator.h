// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

//! Interface for operations providing elapsed time and estimated remaining duration.
class iAguibase_API iADurationEstimator
{
public:
	virtual ~iADurationEstimator();
	//! Get the time that has elapsed since start of the operation.
	//! @return elapsed time in seconds
	virtual double elapsed() const =0;
	//! Get the estimated, still required time to finish the operation.
	//! @return the estimated remaining time in seconds
	//!         -1 if remaining time still unknown
	virtual double estimatedTimeRemaining(double percent) const =0;
};
