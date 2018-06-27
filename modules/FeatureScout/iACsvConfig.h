/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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

#include "iAFeatureScoutObjectType.h"

#include <QString>

//! parameters for csv loading configuraton
struct iACsvConfig
{
	iACsvConfig() :
		fileName(""),
		formatName(""),
		skipLinesStart(5),
		skipLinesEnd(0),
		colSeparator(";"),
		decimalSeparator("."),
		addAutoID(false),
		objectType(iAFeatureScoutObjectType::Voids),
		unit("microns"),
		spacing(0.0f),
		tableWidth(0),
		paramsValid(true)
	{}

	QString fileName;//!< filename, not stored in registrys

	QString formatName;
	size_t skipLinesStart, skipLinesEnd;
	QString colSeparator, decimalSeparator;
	bool addAutoID;
	iAFeatureScoutObjectType objectType;
	//TODO to be applied later
	QString unit;
	float spacing;
	size_t tableWidth;

	bool paramsValid;
};
