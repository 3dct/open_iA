#include "dlg_VisMainWindow.h"

//iA
#include "mainwindow.h"

dlg_VisMainWindow::dlg_VisMainWindow(MainWindow* parent) 
	: QMainWindow(parent)
{
	parent->mdiArea->addSubWindow(this);

	setupUi(this);

}