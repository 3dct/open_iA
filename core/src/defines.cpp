#include "defines.h"

QColor* PredefinedColors()
{
	static QColor PredefColors[7] = {
		QColor(0, 0, 0)
		, QColor(0, 255, 0)
		, QColor(255, 0, 0)
		, QColor(255, 255, 0)
		, QColor(0, 255, 255)
		, QColor(255, 0, 255)
		, QColor(255, 255, 255)
	};
	return PredefColors;
}
