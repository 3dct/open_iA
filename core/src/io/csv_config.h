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

	//parameters for csv loading configuraton
	struct configPararams {

		configPararams(){
			initDefaultParams(); 
		
		};

		void initDefaultParams() {
			fileName = "";
			startLine = 1;
			endLine = 0;
			file_seperator = csvSeparator::Colunm;
			csv_Inputlanguage = inputLang::EN; 
			spacing = 0.0f; 
			file_fmt = csv_FileFormat::Default;
			file_decimalPoint = decimalPoint::Dot;
			csv_units = "microns";
			paramsValid = true; 
		
		}

		QString fileName;
		//QString colSeparator;
		
		unsigned long startLine;
		unsigned long endLine; 

		//TODO to be applied later
		float spacing; 
		
		QString csv_units;
		enum csvSeparator file_seperator; 
		enum csv_FileFormat file_fmt;
		enum inputLang csv_Inputlanguage; 
		enum decimalPoint file_decimalPoint; 
		bool paramsValid;
	};
	
	

}