#pragma once

#include "iAremote_export.h"

#include "vtkRenderWindow.h"
#include <qobject.h>

class iAImagegenerator: QObject
{
	Q_OBJECT

public: 
	static vtkUnsignedCharArray* createImage(vtkRenderWindow* window, int quality);

};

