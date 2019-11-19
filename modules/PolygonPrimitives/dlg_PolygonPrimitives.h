#pragma once

#include <QDialog>
#include "mdichild.h"
#include "ui_PolygonPrimitives.h"
#include "PolyGen.h"

class QString; 
class iARenderer;

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



	//void checkInput()

private slots:
	void createObject(); 


private:
	
	
	void readData(iARenderer* renderer, color aColor);
	
	void readSphereData(iARenderer* renderer, color aColor);
	color getColor() const;


	inline bool checkNullempty(const QString& val) {
		bool res = false; 
		return res = val.isNull() || val.isEmpty();
	
	}
	

	PolyGen visualiser;

	MdiChild *m_child; 
	color m_Color; 
};

