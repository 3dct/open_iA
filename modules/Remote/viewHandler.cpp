#include "viewHandler.h"


viewHandler::viewHandler() {
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, [=]() -> void { createImage(id, 100); });
}

void viewHandler::vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* /*callData*/) {
	auto now = QDateTime::currentMSecsSinceEpoch();
	if ((now - Lastrendered) > 50)
	{
		if (now - Lastrendered < 250)
		{
			timer->stop();
			timer->start(250);
		}
		createImage(id, quality);
		Lastrendered = QDateTime::currentMSecsSinceEpoch();
		timeRendering = Lastrendered - now;
		

	}

};