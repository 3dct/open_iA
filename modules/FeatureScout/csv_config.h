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

#include <QString>

//stores csv config parameters
namespace csvConfig
{
	enum class CTInputObjectType{Fiber =1, Voids = 0};
	const QString DefaultColSeparator(";");
	const QString DefaultDecimalSeparator(".");

	//! parameters for csv loading configuraton
	struct configPararams
	{
		configPararams()
		{
			initDefaultParams();
		}

		void initDefaultParams()
		{
			fileName = "";
			skipLinesStart = 5;
			skipLinesEnd   = 0;
			colSeparator = DefaultColSeparator;
			decimalSeparator = DefaultDecimalSeparator;
			unit = "microns";
			paramsValid = true;
			inputObjectType = CTInputObjectType::Voids;
			tableWidth = 0;
			formatName = "";
			colCount = 0;
			spacing = 0.0f;
		}

		//! filename, not stored in registrys
		QString fileName;

		uint skipLinesStart, skipLinesEnd;
		unsigned long colCount;
		unsigned long tableWidth;

		QString unit;
		QString formatName;
		QString colSeparator, decimalSeparator;
		bool paramsValid;
		CTInputObjectType inputObjectType;

		//TODO to be applied later
		float spacing;
	};
}