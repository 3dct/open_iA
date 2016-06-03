#include <QtGui/QApplication>
#include <QtOpenGL/QGLWidget>
#include <QFile>
#include "../dreamcaster.h"

DreamCaster * dcast_main;
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	//app = new QApplication(argc, argv);
	DreamCaster w;
	dcast_main = &w;
	//w.initRaycast();
	//app->setMainWidget(&w);
	w.show();
	int res = app.exec();
	app.exit();
	//delete app;//TODO: �������, error on calling app destructor, why?
	return res;
}
