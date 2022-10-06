#pragma once

#include <vtkRendererCollection.h>
#include <vtkCallbackCommand.h>

#include <QObject>
#include <QDateTime>
#include <QTimer>


class viewHandler: public QObject
{

	Q_OBJECT
public:
	viewHandler();
	void vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* /*callData*/);

	QString id;
	int quality = 45;

private: 
	long long Lastrendered =0;
	int timeRendering =0;
	QTimer* timer;


Q_SIGNALS:
	void createImage(QString id, int Quality);
};

