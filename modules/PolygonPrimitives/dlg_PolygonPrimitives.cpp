#include "dlg_PolygonPrimitives.h"
#include "QMessageBox"
#include <iAConsole.h>
//#include "PolyGen.h"
#include "iARenderer.h"
#include "vtkOpenGLRenderer.h"

PolygonPrimitives::PolygonPrimitives(QWidget* parent, Qt::WindowFlags f) :QDialog(parent, f) {
	setupUi(this); 
	performConnections(); 

}

PolygonPrimitives::~PolygonPrimitives()
{
	this->m_child = nullptr; 
}

void PolygonPrimitives::performConnections()
{
	connect(btn_addObj, SIGNAL(clicked()), this, SLOT(createObject()));
}

void PolygonPrimitives::createObject()
{
	if (!m_child) return; 
	auto renderer = m_child->renderer();
	bool lineChecked = this->rdBtn_Line->isChecked();
	if (lineChecked) {
		readData(renderer);
	}
	else {
		readSphereData(renderer);
	}
	

	DEBUG_LOG("Action is triggered");

}

void PolygonPrimitives::readData(iARenderer *renderer)
{
	double x1, x2;
	double y1, y2; 
	double z1, z2; 

	QString str_x1, str_x2, str_y1, str_y2, str_z1, str_z2; 
	bool check_x1, check_x2, check_y1, check_y2, check_z1, check_z2; 

	str_x1 = this->ed_x1->text();
	str_x2 = this->ed_X2->text();
		
	str_y1 = this->ed_Y1->text();
	str_y2 = this->ed_Y2->text(); 

	str_z1 = this->ed_Z1->text();
	str_z2 = this->ed_Z2->text();

	check_x1 = this->checkNullempty(str_x1);
	check_x2 = this->checkNullempty(str_x2);

	check_y1 = this->checkNullempty(str_y1);
	check_y2 = this->checkNullempty(str_y2);

	check_z1 = this->checkNullempty(str_z1);
	check_z2 = this->checkNullempty(str_z2);

	if (check_x1 || check_x2 || check_y1 || check_y2 || check_z1 || check_z2) {
		QMessageBox msgBox;
		msgBox.setText("Please Enter coordinates for every element.");
		msgBox.exec();
		return; 
	}

	x1 = str_x1.toDouble();
	x2 = str_x2.toDouble();
		
	y1 = str_y1.toDouble();
	y2 = str_y2.toDouble();

	z1 = str_z1.toDouble();
	z2 = str_z2.toDouble();

	auto oglRenderer = renderer->renderer(); 
	if (!oglRenderer) return; 

	visualiser.createAndRenderLine(oglRenderer, x1, y1, z1, x2, y2, z2, color::green);

	renderer->update();
	DEBUG_LOG("Starting action");
	
}

void PolygonPrimitives::readSphereData(iARenderer* renderer)
{
	if (!renderer) return; 
	double xm, ym, zm; 
	double raduis = 0; 
	auto oglRenderer = renderer->renderer();
	if (!oglRenderer) return; 


	QString str_xm, str_ym, str_zm, str_raduis;
	bool check_xm, check_ym, check_zm, checkRaduis;


	str_xm = this->ed_SphereXm->text();
	str_ym = this->ed_SphereYm->text();
	str_zm = this->ed_SphereZm->text();
	str_raduis = this->ed_SphereCentRaduis->text();

	check_xm = this->checkNullempty(str_xm);
	check_ym = this->checkNullempty(str_ym);

	check_zm = this->checkNullempty(str_zm);
	checkRaduis = this->checkNullempty(str_raduis);

	if (check_xm || check_ym || check_zm || checkRaduis) {
		QMessageBox msgBox;
		msgBox.setText("Please Enter coordinates for every element.");
		msgBox.exec();
		return;
	
	}

	xm = str_xm.toDouble();
	ym = str_ym.toDouble();
	zm = str_zm.toDouble();
	raduis = str_raduis.toDouble();

	visualiser.createAndRenderSphere(oglRenderer, xm, ym, zm, raduis, color::green);
	renderer->update(); 

	
}
