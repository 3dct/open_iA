#pragma once

#include "ui_CompVisMainWindow.h"

//iA
#include "dlg_MultidimensionalScalingDialog.h"
#include "iACsvDataStorage.h"


//QT
#include <QMainWindow>

class iAMultidimensionalScaling;
class iAMainWindow;
class iACompVisMain;

class dlg_VisMainWindow : public QMainWindow, public Ui_CompVisMainWindow
{
	Q_OBJECT

   public:
	dlg_VisMainWindow(QList<csvFileData>* data, iAMultidimensionalScaling* mds, iAMainWindow* parent, iACompVisMain* main);
	QList<csvFileData>* getData();
	void startMDSDialog();

	void recalculateMDS();
	void updateMDS(iAMultidimensionalScaling* newMds);

   private:
	void createMenu();

	void reorderHistogramTableAscending();
	void reorderHistogramTableDescending();
	void reorderHistogramTableAsLoaded();

	void enableUniformTable();
	void enableBayesianBlocks();
	void enableNaturalBreaks();
	void enableCurveTable();

	iACompVisMain* m_main;
	QList<csvFileData>* m_data;
	iAMultidimensionalScaling* m_mds;
	dlg_MultidimensionalScalingDialog* m_MDSD;
	

   private slots:
};
