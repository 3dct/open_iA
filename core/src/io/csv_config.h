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
namespace csvConfig {
	//which file format apply - default, vg, mavi, featurescout
	enum class csv_FileFormat { Default = 1, VolumeGraphics, MAVI, open_IA_FeatureScout };
	//parameters for csv loading configuraton
	//seperator in csv file
	enum class csvSeparator{Colunm = 1, Comma };
	enum class decimalPoint{Dot = 1, Comma  };
	enum class inputLang{EN = 1, GER};
	enum class CTInputObjectType{Fiber =1, Voids = 0};

	//parameters for csv loading configuraton
	struct configPararams {

		configPararams()
		{
			initDefaultParams();

		};

		void initDefaultParams()
		{
			fileName = "";
			skipLinesStart = 5;
			skipLinesEnd   = 0;
			headerStartLine = 5;
			colCount = 31;
			setDefaultConfigs();
		}

		void setDefaultConfigs()
		{
			file_seperator = csvSeparator::Colunm;
			csv_Inputlanguage = inputLang::EN;
			spacing = 0.0f;
			file_fmt = csv_FileFormat::Default;
			file_decimalPoint = decimalPoint::Dot;
			csv_units = "microns";
			paramsValid = true;
			inputObjectType = CTInputObjectType::Voids;
			tableWidth = 0;
			LayoutName = "";
		}

		void resetParams()
		{
			skipLinesStart = 0;
			skipLinesEnd = 0;
			headerStartLine = 0;
			colCount = 31;
			setDefaultConfigs();
		}

		QString fileName;

		uint skipLinesStart, skipLinesEnd;
		unsigned long headerStartLine;
		unsigned long colCount;
		unsigned long tableWidth;

		//TODO to be applied later
		float spacing;

		QString csv_units;
		QString LayoutName;
		csvSeparator file_seperator;
		csv_FileFormat file_fmt;
		inputLang csv_Inputlanguage;
		decimalPoint file_decimalPoint;
		bool paramsValid;
		bool useEndline;
		CTInputObjectType inputObjectType;
	};
}