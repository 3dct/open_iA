#pragma once

#include <QDialog>
#include "ui_PolygonPrimitives.h"

class PolygonPrimitives : public QDialog, Ui_PolygonPrimitives
{
Q_OBJECT

public:
	PolygonPrimitives(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~PolygonPrimitives();

};

