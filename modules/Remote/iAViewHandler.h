// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkRendererCollection.h>
#include <vtkCallbackCommand.h>

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>


class iAViewHandler: public QObject
{

	Q_OBJECT
public:
	iAViewHandler();
	void vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* /*callData*/);

	QString id;
	int quality = 45;

private: 
	long long Lastrendered =0;
	int timeRendering =0;
	int waitTimeRendering = 50;
	QTimer* timer;
	QElapsedTimer m_StoppWatch;

Q_SIGNALS:
	void createImage(QString id, int Quality);
};
