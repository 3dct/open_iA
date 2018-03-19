#pragma once
#include <qstring.h>

//stores csv config parameters
namespace csvConfig {
	//which file format apply - default, vg, mavi, featurescout
	enum class csv_FileFormat { Default = 1, VolumeGraphics, MAVI, open_IA_FeatureScout };
	//parameters for csv loading configuraton
	struct configPararams {

		configPararams() : fileName(""), startLine(1), colSeparator(","), fmt_ENG(false),paramsValid(false), spacing(0.0f), file_fmt(csvConfig::csv_FileFormat::Default) {};

		QString fileName;
		
		QString colSeparator;
		unsigned long startLine;
		
		//TODO to be applied later
		float spacing; 
		bool fmt_ENG;
	/*bool VG_FileSelected;*/
		bool paramsValid;

		
		enum csv_FileFormat file_fmt; 
	};
	
	

}