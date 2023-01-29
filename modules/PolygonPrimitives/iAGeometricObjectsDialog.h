// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

#include "ui_PolygonPrimitives.h"

class iAMdiChild;

class vtkOpenGLRenderer;

class QColor;
class QString;

#include <vtkSmartPointer.h>

class vtkPolyDataAlgorithm;

class iAGeometricObjectsDialog : public QDialog, Ui_PolygonPrimitives
{
Q_OBJECT

public:
	iAGeometricObjectsDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	void setMDIChild(iAMdiChild* child);

private slots:
	void createObject();
	void updateControls();

private:
	vtkSmartPointer<vtkPolyDataAlgorithm> createLineSource ();
	vtkSmartPointer<vtkPolyDataAlgorithm> createSphereSource();
	vtkSmartPointer<vtkPolyDataAlgorithm> createCubeSource ();

	iAMdiChild *m_child;
};
