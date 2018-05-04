#pragma once
#include <qstring.h>

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

		configPararams(){
			initDefaultParams();

		};

		void initDefaultParams() {
			fileName = "";
			startLine = 5;
			headerStartLine = 5;
			colCount = 31;
			useEndline = false;
			endLine = 0;
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


		void resetParams() {
			startLine = 0;
			headerStartLine = 0;
			colCount = 31;
			useEndline = false;
			endLine = 0;
			setDefaultConfigs();
		}

		QString fileName;


		unsigned long startLine;

		unsigned long headerStartLine;
		unsigned long endLine;
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