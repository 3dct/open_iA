#pragma once

#include "ui_CompVisMainWindow.h"
//iA

//QT
#include <QMainWindow>

class MainWindow;

class dlg_VisMainWindow : public QMainWindow, public Ui_CompVisMainWindow
{
	Q_OBJECT

   public:
	dlg_VisMainWindow(MainWindow* parent = 0);

   private:

	private slots:
};
