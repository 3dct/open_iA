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

	void createAndRenderLine(vtkOpenGLRenderer* renderer, double x1, double y1, double z1,
		double x2, double y2, double z2, double lnWithd, QColor const& color)
	{
		if (lnWithd <= 0)
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
		lineProp->SetLineWidth(lnWithd);
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
		sphereProp->SetLineWidth(100.0);
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
		/*sphereProp->SetLineWidth(100.0);*/
		cubeProp->SetColor(color.redF(), color.greenF(), color.blueF());
		actor->SetMapper(mapper);
		renderer->AddActor(actor);
	}

}

iAGeometricObjectsDialog::iAGeometricObjectsDialog(QWidget* parent, Qt::WindowFlags f) :QDialog(parent, f)
{
	setupUi(this);
	connect(btn_addObj, SIGNAL(clicked()), this, SLOT(createObject()));
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
	if (!m_child) return;
	auto renderer = m_child->renderer();
	if (!renderer) return;
	auto oglRenderer = renderer->renderer();
	if (!oglRenderer) return;
	bool sphereChecked = this->rdBtn_Sphere->isChecked();
	bool lineChecked = this->rdBtn_Line->isChecked();
	//bool cubeChecked = this->rdBtn_Cube->isChecked();

	auto colorStr = this->cmbBox_col->currentText();
	QColor color(colorStr);

	if (lineChecked)
	{
		readLineData(oglRenderer, color);
	}
	else if (sphereChecked)
	{
		readSphereData(oglRenderer, color);
	}
	else
	{
		readCubeData(oglRenderer, color);
	}
	renderer->update();
}

void iAGeometricObjectsDialog::readLineData(vtkOpenGLRenderer* oglRenderer, QColor const & color)
{
	bool check_x1, check_x2, check_y1, check_y2, check_z1, check_z2, check_thickness;
	double x1 = this->ed_x1->text().toDouble(&check_x1);
	double x2 = this->ed_x2->text().toDouble(&check_x2);
	double y1 = this->ed_y1->text().toDouble(&check_y1);
	double y2 = this->ed_y2->text().toDouble(&check_y2);
	double z1 = this->ed_z1->text().toDouble(&check_z1);
	double z2 = this->ed_z2->text().toDouble(&check_z2);
	double thickness = this->ed_Thickness->text().toDouble(&check_thickness);
	if (!check_x1 || !check_x2 || !check_y1 || !check_y2 || !check_z1 || !check_z2 || !check_thickness)
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter valid coordinates for every element.");
		msgBox.exec();
		return;
	}
	createAndRenderLine(oglRenderer, x1, y1, z1, x2, y2, z2, thickness, color);
}

void iAGeometricObjectsDialog::readSphereData(vtkOpenGLRenderer* oglRenderer, QColor const & color)
{
	bool check_xm, check_ym, check_zm, check_radius;
	double xm = ed_SphereXm->text().toDouble(&check_xm);
	double ym = ed_SphereYm->text().toDouble(&check_ym);
	double zm = ed_SphereZm->text().toDouble(&check_zm);
	double radius = ed_Thickness->text().toDouble(&check_radius);
	if (!check_xm || !check_ym || !check_zm || !check_radius)
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter valid coordinates for every element.");
		msgBox.exec();
		return;
	}
	createAndRenderSphere(oglRenderer, xm, ym, zm, radius, color);
}

void iAGeometricObjectsDialog::readCubeData(vtkOpenGLRenderer* oglRenderer, QColor const& color)
{
	bool check_xmax, check_xmin, check_ymin, check_ymax, check_zmax, check_zmin;
	double xmin = this->ed_cubeXmin->text().toDouble(&check_xmin);
	double xmax = this->ed_cubeXmax->text().toDouble(&check_xmax);
	double ymin = this->ed_cubeYmin->text().toDouble(&check_ymin);
	double ymax = this->ed_cubeYmax->text().toDouble(&check_ymax);
	double zmin = this->ed_cubeZmin->text().toDouble(&check_zmin);
	double zmax = this->ed_cubeZmax->text().toDouble(&check_zmax);
	if (!check_xmin || !check_xmax || !check_ymin || !check_ymax || !check_zmin || !check_zmax)
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter valid coordinates for every element.");
		msgBox.exec();
		return;
	}
	createAndRenderCube(oglRenderer, xmin, ymin, zmin, xmax, ymax, zmax, color);
}
