#include "Globals.h"
#include "XmdvTool.h"
//#include "color/ColorManager.h"
#include <QFont>

#include <string>

Globals::Globals()
{
	initTextFont();
	default_file_name1 = "cars.okc";
	default_file_name2 = "iris.okc";
	//colorManager = new ColorManager();
}

Globals::~Globals()
{
	//SAFE_DELETE(colorManager);
}

void Globals::initTextFont() {
	textFont = QFont("Helvetica", 10);
}
