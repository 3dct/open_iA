/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"

#include "iAValueType.h"

#include <cstddef> // for size_t
#include <cmath>   // for log

class open_iA_Core_API iAAbstractDiagramData
{
public:
	typedef double DataType;
	virtual ~iAAbstractDiagramData() {}
	virtual DataType const * GetData() const =0;
	virtual size_t GetNumBin() const =0;
	virtual double GetMinX() const { return 0; }
	virtual double GetMaxX() const { return GetNumBin(); }
};


namespace
{
	//! Logarithmic base used for diagram axes
	const double LogBase = 2.0;
}

/**
 * Logarithmic convenience function for axes, using base above
 */
template <typename T>
T LogFunc(T value)
{
	return std::log(value) / std::log(LogBase);
}

class iAAbstractDiagramRangedData: public iAAbstractDiagramData
{
public:
	virtual double GetSpacing() const =0;
	virtual double * GetDataRange() =0;
	virtual double GetDataRange(int idx) const =0;
	virtual DataType GetMaxValue() const =0;
	virtual DataType GetMinValue() const { return 0; }

	virtual double GetBinStart(int binNr) const		// default: assume constant (i.e. linear) spacing
	{
		return GetSpacing() * binNr + GetDataRange(0);
	}
	virtual iAValueType GetRangeType() const
	{
		return Continuous;
	}
};
