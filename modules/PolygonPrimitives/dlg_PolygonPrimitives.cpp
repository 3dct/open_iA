#include "dlg_PolygonPrimitives.h"
#include "QMessageBox"
#include <iAConsole.h>
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

	auto renderer = m_child->renderer()->renderer();

	DEBUG_LOG("Action is triggered");
	QMessageBox msgBox;
	msgBox.setText("just an action.");
	msgBox.exec();
}
