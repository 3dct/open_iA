#pragma once

#include "mainwindow.h"

#include "iACsvDataStorage.h"
#include "dlg_VisMainWindow.h"

//QT
#include"qlist.h"

class iAMultidimensionalScaling;

class iACompVisMain
{
   public:
	iACompVisMain(MainWindow* mainWin);
	//load the CSV datasets
	void loadData();
	
   private:
	void initializeMDS();

	dlg_VisMainWindow* m_mainW;
	QList<csvFileData>* m_data;
	iAMultidimensionalScaling* m_mds;
};