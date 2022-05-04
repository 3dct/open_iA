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
	dlg_VisMainWindow(iACsvDataStorage* dataStorage, iAMultidimensionalScaling* mds, iAMainWindow* parent,
		iACompVisMain* main, bool computeMDSFlag);
	QList<csvFileData>* getData();
	void startMDSDialog();

	void recalculateMDS();
	void updateMDS(iAMultidimensionalScaling* newMds);

	//deactivates the ordering button
	void deactivateOrdering();
	//activates the ordering button
	void activateOrdering();

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

	iACsvDataStorage* m_dataStorage;

	bool m_computeMDSFlag;
	

   private slots:
};
