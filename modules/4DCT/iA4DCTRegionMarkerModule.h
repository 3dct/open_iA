// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iAVisModule.h"

class iA4DCTRegionMarkerModule : public iAVisModule
{
public:
			iA4DCTRegionMarkerModule( );
	void	show( ) override;
	void	hide( ) override;
	void	addRegion( double* pos );
};
