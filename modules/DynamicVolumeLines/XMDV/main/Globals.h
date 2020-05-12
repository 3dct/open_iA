#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <QFont>

#include <string>

class OkcData;
class ColorManager;

class Globals
{
public:
	Globals();
	virtual ~Globals();

	QFont textFont;
	// The file names for the default datasets that will be opened
	// at the first time when the XmdvTool are opened.
	std::string	default_file_name1;
	std::string	default_file_name2;
	ColorManager *colorManager;

private:
	void initTextFont();

};

#endif /*GLOBALS_H_*/
