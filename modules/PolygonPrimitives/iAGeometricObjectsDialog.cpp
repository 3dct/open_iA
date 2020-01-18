/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
	vtkSmartPointer<vtkActor> createActor()
	{
		auto actor = vtkSmartPointer<vtkActor>::New();
		actor->SetDragable(false);
		actor->SetPickable(false);
		return actor;
	}

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

	void createAndRenderLine(vtkOpenGLRenderer* renderer, iAVec3d & pt1, iAVec3d & pt2, float lineWidth, QColor const& color)
	{
		if (lineWidth <= 0)
		{
			DEBUG_LOG("Line width must be bigger than 0!");
			return;
		}
		auto lineSource = vtkSmartPointer<vtkLineSource>::New();
		lineSource->SetPoint1(pt1.data()); // VTK problem: should take const data!
		lineSource->SetPoint2(pt2.data());
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(lineSource->GetOutputPort());
		auto actor = createActor();
		actor->SetOrigin(0, 0, 0);
		actor->SetOrientation(0, 0, 0);
		auto lineProp = actor->GetProperty();
		lineProp->SetLineWidth(lineWidth);
		lineProp->SetColor(color.redF(), color.greenF(), color.blueF());
		actor->SetMapper(mapper);
		renderer->AddActor(actor);
	}

	void createAndRenderSphere(vtkOpenGLRenderer* renderer, iAVec3d & center, double radius,
		QColor const& color)
	{
		auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
		sphereSource->SetCenter(center.data());
		sphereSource->SetRadius(radius);
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(sphereSource->GetOutputPort());
		auto actor = createActor();
		auto sphereProp = actor->GetProperty();
		sphereProp->SetColor(color.redF(), color.greenF(), color.blueF());
		actor->SetMapper(mapper);
		renderer->AddActor(actor);
	}

	void createAndRenderCube(vtkOpenGLRenderer* renderer, iAVec3d & ptmin, iAVec3d & ptmax, QColor const& color)
	{
		auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
		cubeSource->SetBounds(ptmin.x(), ptmax.x(), ptmin.y(), ptmax.y(), ptmin.z(), ptmax.z());
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(cubeSource->GetOutputPort());
		auto actor = createActor();
		auto cubeProp = actor->GetProperty();
		cubeProp->SetColor(color.redF(), color.greenF(), color.blueF());
		actor->SetMapper(mapper);
		renderer->AddActor(actor);
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
	connect(pbAddObject, SIGNAL(clicked()), this, SLOT(createObject()));
	connect(rbSphere, &QRadioButton::toggled, this, &iAGeometricObjectsDialog::updateControls);
	connect(rbLine, &QRadioButton::toggled, this, &iAGeometricObjectsDialog::updateControls);
	connect(rbCube, &QRadioButton::toggled, this, &iAGeometricObjectsDialog::updateControls);
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

	if (lineChecked)
	{
		readLineData(oglRenderer, color);
	}
	else if (sphereChecked)
	{
		readSphereData(oglRenderer, color);
	}
	else if (cubeChecked)
	{
		readCubeData(oglRenderer, color);
	}
	else
	{
		DEBUG_LOG("Invalid state!");
	}
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

void iAGeometricObjectsDialog::readLineData(vtkOpenGLRenderer* oglRenderer, QColor const & color)
{
	bool check_width;
	iAVec3d pt1, pt2;
	QString pt1str[3] = {edPt1x->text(), edPt1y->text(), edPt1z->text()};
	QString pt2str[3] = {edPt2x->text(), edPt2y->text(), edPt2z->text()};
	bool pt1OK = readPointData(pt1, pt1str);
	bool pt2OK = readPointData(pt2, pt2str);
	float width = edLineWidth->text().toFloat(&check_width);
	if (!pt1OK || !pt2OK || !check_width)
	{
		QMessageBox::warning(this, "Polygon Object", "Please enter valid coordinates for every element.");
		return;
	}
	createAndRenderLine(oglRenderer, pt1, pt2, width, color);
}

void iAGeometricObjectsDialog::readSphereData(vtkOpenGLRenderer* oglRenderer, QColor const & color)
{
	bool check_radius;
	iAVec3d pt1;
	QString pt1str[3] = {edPt1x->text(), edPt1y->text(), edPt1z->text()};
	bool pt1OK = readPointData(pt1, pt1str);
	double radius = edRadius->text().toDouble(&check_radius);
	if (!pt1OK || !check_radius)
	{
		QMessageBox::warning(this, "Polygon Object", "Please enter valid coordinates for every element.");
		return;
	}
	createAndRenderSphere(oglRenderer, pt1, radius, color);
}

void iAGeometricObjectsDialog::readCubeData(vtkOpenGLRenderer* oglRenderer, QColor const& color)
{
	iAVec3d pt1, pt2;
	QString pt1str[3] = {edPt1x->text(), edPt1y->text(), edPt1z->text()};
	QString pt2str[3] = {edPt2x->text(), edPt2y->text(), edPt2z->text()};
	bool pt1OK = readPointData(pt1, pt1str);
	bool pt2OK = readPointData(pt2, pt2str);
	if (!pt1OK || !pt2OK)
	{
		QMessageBox::warning(this, "Polygon Object", "Please enter valid coordinates for every element.");
		return;
	}
	createAndRenderCube(oglRenderer, pt1, pt2, color);
}
