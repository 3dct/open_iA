// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGeometricObjectsDialog.h"

#include <iALog.h>
#include <iAVec3.h>
#include <iAMdiChild.h>

#include "iAGeometricObject.h"

#include <vtkLineSource.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>

#include <QColor>
#include <QMessageBox>

namespace
{
	vtkSmartPointer<vtkPolyDataAlgorithm> createLine(iAVec3d & pt1, iAVec3d & pt2)
	{
		auto lineSource = vtkSmartPointer<vtkLineSource>::New();
		lineSource->SetPoint1(pt1.data()); // VTK problem: should take const data!
		lineSource->SetPoint2(pt2.data());
		return lineSource;
	}

	vtkSmartPointer<vtkPolyDataAlgorithm> createSphere(iAVec3d & center, double radius)
	{
		auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
		sphereSource->SetCenter(center.data());
		sphereSource->SetRadius(radius);
		return sphereSource;
	}

	vtkSmartPointer<vtkPolyDataAlgorithm> createCube(iAVec3d & ptmin, iAVec3d & ptmax)
	{
		auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
		cubeSource->SetBounds(ptmin.x(), ptmax.x(), ptmin.y(), ptmax.y(), ptmin.z(), ptmax.z());
		return cubeSource;
	}

	bool readPointData(iAVec3d & pt, QString coords[3])
	{
		bool ok[3];
		for (size_t i=0; i<3; ++i)
		{
			pt[i] = coords[i].toDouble(&ok[i]);
		}
		return ok[0] && ok[1] && ok[2];
	}
}

iAGeometricObjectsDialog::iAGeometricObjectsDialog(QWidget* parent, Qt::WindowFlags f) :QDialog(parent, f)
{
	setupUi(this);
	connect(pbAddObject, &QPushButton::clicked, this, &iAGeometricObjectsDialog::createObject);
	connect(rbSphere, &QRadioButton::toggled, this, &iAGeometricObjectsDialog::updateControls);
	connect(rbLine, &QRadioButton::toggled, this, &iAGeometricObjectsDialog::updateControls);
	connect(rbCube, &QRadioButton::toggled, this, &iAGeometricObjectsDialog::updateControls);
	connect(pbClose, &QPushButton::clicked, this, &iAGeometricObjectsDialog::accept);
	updateControls();
}

void iAGeometricObjectsDialog::setMDIChild(iAMdiChild* child)
{
	m_child = child;
	connect(m_child, &iAMdiChild::closed, [this]
	{   // if child is closed, disassociate with that child and close dialog
		m_child = nullptr;
		hide();
	});
}

void iAGeometricObjectsDialog::createObject()
{
	if (!m_child)
	{
		return;
	}
	bool sphereChecked = rbSphere->isChecked();
	bool lineChecked   = rbLine->isChecked();
	bool cubeChecked   = rbCube->isChecked();
	vtkSmartPointer<vtkPolyDataAlgorithm> source;
	QString name;
	if (lineChecked)
	{
		name = "Line";
		source = createLineSource();
	}
	else if (sphereChecked)
	{
		name = "Sphere";
		source = createSphereSource();
	}
	else if (cubeChecked)
	{
		name = "Cube";
		source = createCubeSource();
	}
	if (!source)
	{
		return;
	}
	auto dataSet = std::make_shared<iAGeometricObject>(name, source);
	m_child->addDataSet(dataSet);
}

void iAGeometricObjectsDialog::updateControls()
{
	bool sphereChecked = rbSphere->isChecked();
	bool lineChecked   = rbLine->isChecked();
	bool cubeChecked   = rbCube->isChecked();

	lbPt3 ->setVisible(false);
	edPt3x->setVisible(false);
	edPt3y->setVisible(false);
	edPt3z->setVisible(false);
	lbPt2 ->setVisible(!sphereChecked);
	edPt2x->setVisible(!sphereChecked);
	edPt2y->setVisible(!sphereChecked);
	edPt2z->setVisible(!sphereChecked);
	lbRadius->setVisible(sphereChecked);
	edRadius->setVisible(sphereChecked);
	if (sphereChecked)
	{
		lbPt1->setText("Center");
	}
	else if (lineChecked)
	{
		lbPt1->setText("Start");
		lbPt2->setText("End");
	}
	else if (cubeChecked)
	{
		lbPt1->setText("Min");
		lbPt2->setText("Max");
	}
}

vtkSmartPointer<vtkPolyDataAlgorithm> iAGeometricObjectsDialog::createLineSource()
{
	iAVec3d pt1, pt2;
	QString pt1str[3] = {edPt1x->text(), edPt1y->text(), edPt1z->text()};
	QString pt2str[3] = {edPt2x->text(), edPt2y->text(), edPt2z->text()};
	bool pt1OK = readPointData(pt1, pt1str);
	bool pt2OK = readPointData(pt2, pt2str);
	if (!pt1OK || !pt2OK)
	{
		QMessageBox::warning(this, "Object", "Please enter valid coordinates for every element.");
		return vtkSmartPointer<vtkPolyDataAlgorithm>();
	}
	return createLine(pt1, pt2);
}

vtkSmartPointer<vtkPolyDataAlgorithm> iAGeometricObjectsDialog::createSphereSource()
{
	bool check_radius;
	iAVec3d pt1;
	QString pt1str[3] = {edPt1x->text(), edPt1y->text(), edPt1z->text()};
	bool pt1OK = readPointData(pt1, pt1str);
	double radius = edRadius->text().toDouble(&check_radius);
	if (!pt1OK || !check_radius)
	{
		QMessageBox::warning(this, "Object", "Please enter valid coordinates for every element.");
		return vtkSmartPointer<vtkPolyDataAlgorithm>();
	}
	return createSphere(pt1, radius);
}

vtkSmartPointer<vtkPolyDataAlgorithm> iAGeometricObjectsDialog::createCubeSource()
{
	iAVec3d pt1, pt2;
	QString pt1str[3] = {edPt1x->text(), edPt1y->text(), edPt1z->text()};
	QString pt2str[3] = {edPt2x->text(), edPt2y->text(), edPt2z->text()};
	bool pt1OK = readPointData(pt1, pt1str);
	bool pt2OK = readPointData(pt2, pt2str);
	if (!pt1OK || !pt2OK)
	{
		QMessageBox::warning(this, "Object", "Please enter valid coordinates for every element.");
		return vtkSmartPointer<vtkPolyDataAlgorithm>();
	}
	return createCube(pt1, pt2);
}
