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

	void createAndRenderLine(vtkOpenGLRenderer* renderer, double x1, double y1, double z1,
		double x2, double y2, double z2, double lineWidth, QColor const& color)
	{
		if (lineWidth <= 0)
		{
			DEBUG_LOG("Line width must be bigger than 0!");
			return;
		}
		auto lineSource = vtkSmartPointer<vtkLineSource>::New();
		lineSource->SetPoint1(x1, y1, z1);
		lineSource->SetPoint2(x2, y2, z2);
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

	void createAndRenderSphere(vtkOpenGLRenderer* renderer, double xm, double ym, double zm, double radius,
		QColor const& color)
	{
		auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
		sphereSource->SetCenter(xm, ym, zm);
		sphereSource->SetRadius(radius);
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(sphereSource->GetOutputPort());
		auto actor = createActor();
		auto sphereProp = actor->GetProperty();
		sphereProp->SetColor(color.redF(), color.greenF(), color.blueF());
		actor->SetMapper(mapper);
		renderer->AddActor(actor);
	}

	void createAndRenderCube(vtkOpenGLRenderer* renderer, double xmin, double ymin, double zmin,
		double xmax, double ymax, double zmax, QColor const& color)
	{
		auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
		cubeSource->SetBounds(xmin, xmax, ymin, ymax, zmin, zmax);
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(cubeSource->GetOutputPort());
		auto actor = createActor();
		auto cubeProp = actor->GetProperty();
		cubeProp->SetColor(color.redF(), color.greenF(), color.blueF());
		actor->SetMapper(mapper);
		renderer->AddActor(actor);
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
	lbPt2 ->setVisible(false);
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
	bool check_x1, check_x2, check_y1, check_y2, check_z1, check_z2, check_width;
	double x1 = edPt1x->text().toDouble(&check_x1);
	double y1 = edPt1y->text().toDouble(&check_y1);
	double z1 = edPt1z->text().toDouble(&check_z1);

	double x2 = edPt2x->text().toDouble(&check_x2);
	double y2 = edPt2y->text().toDouble(&check_y2);
	double z2 = edPt2z->text().toDouble(&check_z2);
	double width = edLineWidth->text().toDouble(&check_width);
	if (!check_x1 || !check_x2 || !check_y1 || !check_y2 || !check_z1 || !check_z2 || !check_width)
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter valid coordinates for every element.");
		msgBox.exec();
		return;
	}
	createAndRenderLine(oglRenderer, x1, y1, z1, x2, y2, z2, width, color);
}

void iAGeometricObjectsDialog::readSphereData(vtkOpenGLRenderer* oglRenderer, QColor const & color)
{
	bool check_x1, check_y1, check_z1, check_radius;
	double x1 = edPt1x->text().toDouble(&check_x1);
	double y1 = edPt1y->text().toDouble(&check_y1);
	double z1 = edPt1z->text().toDouble(&check_z1);
	double radius = edRadius->text().toDouble(&check_radius);
	if (!check_x1 || !check_y1 || !check_z1 || !check_radius)
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter valid coordinates for every element.");
		msgBox.exec();
		return;
	}
	createAndRenderSphere(oglRenderer, x1, y1, z1, radius, color);
}

void iAGeometricObjectsDialog::readCubeData(vtkOpenGLRenderer* oglRenderer, QColor const& color)
{
	bool check_x1, check_x2, check_y1, check_y2, check_z1, check_z2;
	//QPointF pt1;
	double x1 = edPt1x->text().toDouble(&check_x1);
	double y1 = edPt1y->text().toDouble(&check_y1);
	double z1 = edPt1z->text().toDouble(&check_z1);

	double x2 = edPt2x->text().toDouble(&check_x2);
	double y2 = edPt2y->text().toDouble(&check_y2);
	double z2 = edPt2z->text().toDouble(&check_z2);
	if (!check_x1 || !check_x2 || !check_y1 || !check_y2 || !check_z1 || !check_z2)
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter valid coordinates for every element.");
		msgBox.exec();
		return;
	}
	createAndRenderCube(oglRenderer, x1, y1, z1, x2, y2, z2, color);
}
