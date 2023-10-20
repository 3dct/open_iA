// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

class vtkObject;

class iAViewHandler: public QObject
{
	Q_OBJECT
public:
	iAViewHandler(QString const & id);
	void vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* /*callData*/);

private:
	QString m_id;
	int m_waitTimeRendering = 50;
	QTimer m_timer;
	QElapsedTimer m_stopWatch;

signals:
	void createImage(QString id, int Quality);
};
