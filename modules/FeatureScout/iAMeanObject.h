// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QList>
#include <QObject>
#include <QSharedPointer>

class iAMeanObjectData;
class iAMeanObjectDockWidget;
class iAMdiChild;
class iAQVTKWidget;

class vtkCamera;
class vtkTable;

class QColor;
class QDialog;
class QDockWidget;

class iAMeanObject: public QObject
{
	Q_OBJECT
public:
	iAMeanObject(iAMdiChild* activeChild, QString const& sourcePath);
	void render(QStringList const & classNames, QList<vtkSmartPointer<vtkTable>> const& tableList,
		int filterID, QDockWidget* nextToDW, vtkCamera* commonCamera, QList<QColor> const & classColor);
private slots:
	void modifyMeanObjectTF();
	void saveStl();
	void saveVolume();
private:
	iAMeanObjectDockWidget* m_dwMO;
	QDialog* m_motfView;
	QSharedPointer<iAMeanObjectData> m_MOData;
	iAQVTKWidget* m_meanObjectWidget;
	iAMdiChild* m_activeChild;
	int m_filterID;
	QString m_sourcePath;
};
