/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAGeometricObjectsDialog.h"

#include <iAConsole.h>
#include <iARenderer.h>
#include <iAvec3.h>
#include <mdichild.h>

#include <vtkLineSource.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkOpenGLRenderer.h>
#include <vtkProperty.h>

#include <QColor>
#include <QMessageBox>

namespace
{
	/*
	class iAPolyObject
	{
		vtkSmartPointer<vtkPolyDataAlgorithm> createObject()
	};

	iAPolyObject* createObject(int type)
	{
		switch (type)
	}
	*/

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
	connect(cbWireframe, &QCheckBox::toggled, this, &iAGeometricObjectsDialog::updateControls);
	connect(slOpacity, &QSlider::valueChanged, this, &iAGeometricObjectsDialog::opacityChanged);
	connect(pbClose, &QPushButton::clicked, this, &iAGeometricObjectsDialog::accept);
	updateControls();
}

void iAGeometricObjectsDialog::setMDIChild(MdiChild* child)
{
	m_child = child;
	connect(m_child, &MdiChild::closed, [this]
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
	auto renderer = m_child->renderer();
	auto oglRenderer = renderer->renderer();
	bool sphereChecked = rbSphere->isChecked();
	bool lineChecked   = rbLine->isChecked();
	bool cubeChecked   = rbCube->isChecked();

	auto colorStr = cbColor->currentText();
	QColor color(colorStr);
	float lineWidth = 0;
	if (cbWireframe->isChecked() || lineChecked)
	{
		bool check_width;
		lineWidth = edLineWidth->text().toFloat(&check_width);
		if (lineWidth <= 0)
		{
			QMessageBox::warning(this, "Object", "Line width must be bigger than 0!");
			return;
		}
	}
	double opacity = slOpacity->value() / 10.0;
	vtkSmartPointer<vtkPolyDataAlgorithm> source;
	if (lineChecked)
	{
		source = createLineSource();
	}
	else if (sphereChecked)
	{
		source = createSphereSource();
	}
	else if (cubeChecked)
	{
		source = createCubeSource();
	}
	if (!source)
	{
		return;
	}
	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(source->GetOutputPort());
	auto actor = vtkSmartPointer<vtkActor>::New();
	actor->SetDragable(false);
	actor->SetPickable(false);
	actor->SetOrigin(0, 0, 0);
	actor->SetOrientation(0, 0, 0);
	auto prop = actor->GetProperty();
	if (cbWireframe->isChecked() || lineChecked)
	{
		prop->SetLineWidth(lineWidth);
		prop->SetRepresentationToWireframe();
	}
	prop->SetColor(color.redF(), color.greenF(), color.blueF());
	prop->SetOpacity(opacity);
	actor->SetMapper(mapper);
	oglRenderer->AddActor(actor);
	renderer->update();
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
	cbWireframe->setVisible(!lineChecked);
	edLineWidth->setVisible(lineChecked || cbWireframe->isChecked());
	lbLineWidth->setVisible(lineChecked || cbWireframe->isChecked());
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

void iAGeometricObjectsDialog::opacityChanged(int newValue)
{
	lbOpacityValue->setText(QString("%1").arg(newValue/10.0, 0, 'f', 1) );
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
