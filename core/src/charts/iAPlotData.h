/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"

#include "iAValueType.h"

#include <cstddef> // for size_t
#include <cmath>   // for log

class open_iA_Core_API iAPlotData
{
public:
	typedef double DataType;
	virtual ~iAPlotData() {}
	virtual DataType const * GetRawData() const =0;
	virtual size_t GetNumBin() const =0;
	virtual double GetMinX() const { return 0; }
	virtual double GetMaxX() const { return GetNumBin(); }
	virtual double GetSpacing() const = 0;
	virtual double const * XBounds() const = 0;
	virtual DataType const * YBounds() const = 0;

	virtual double GetBinStart(int binNr) const		// default: assume constant (i.e. linear) spacing
	{
		return GetSpacing() * binNr + XBounds()[0];
	}
	virtual iAValueType GetRangeType() const
	{
		return Continuous;
	}
};
