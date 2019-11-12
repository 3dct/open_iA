#pragma once

#include <QDialog>
#include "ui_PolygonPrimitives"

class PolygonPrimitives : public QDialog, Ui_PolygonPrimitives
{
	PolygonPrimitives(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~PolygonPrimitives();

};

