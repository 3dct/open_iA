#pragma once

#include <QDialog>
#include "mdichild.h"
#include "ui_PolygonPrimitives.h"

class PolygonPrimitives : public QDialog, Ui_PolygonPrimitives
{
Q_OBJECT

public:
	PolygonPrimitives(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~PolygonPrimitives();

	void performConnections();

	inline void setMDIChild(MdiChild* child)
	{
		m_child = child;
	}


private slots:
	void createObject(); 


private:
	MdiChild *m_child; 

};

