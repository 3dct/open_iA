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

	visualiser.createAndRenderObject(oglRenderer, x1, y1, z1, x2, y2, z2, color::green);

	renderer->update();
	DEBUG_LOG("Starting action");
	
}
