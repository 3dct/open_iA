#include "dlg_PolygonPrimitives.h"

PolygonPrimitives::PolygonPrimitives(QWidget* parent, Qt::WindowFlags f) :QDialog(parent, f) {
	setupUi(this); 

}

PolygonPrimitives::~PolygonPrimitives()
{
	this->m_child = nullptr; 
}
