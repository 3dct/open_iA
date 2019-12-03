#pragma once
#include "iAModuleInterface.h"
#include "mainwindow.h"

#include <QMessageBox>


class iAPolygonPrimitivesModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void TestAction();
};