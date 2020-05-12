#include "iACompBoxPlot.h"

//iA
#include "mainwindow.h"

iACompBoxPlot::iACompBoxPlot(MainWindow* parent) : QDockWidget(parent)
{
	//TODO draw boxplot
	setupUi(this);

	this->setFeatures(DockWidgetVerticalTitleBar);
}