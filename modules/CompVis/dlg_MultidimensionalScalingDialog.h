#pragma once
//CompVis
#include "ui_MultidimensionalScalingDialog.h"
//iA
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
//Qt
#include "qdialog.h"
#include <QCheckBox>

class dlg_MultidimensionalScalingDialog : public QDialog, public Ui_MultidimensionalScalingDialog
{
	Q_OBJECT

   public:
	dlg_MultidimensionalScalingDialog(
		QList<csvFileData>* data, iAMultidimensionalScaling* mds, QWidget* parent = 0, Qt::WindowFlags f = 0);

   public slots:
	void onCellChanged(int row, int column);
	//! handles a click on the OK button
	void okBtnClicked();

   private:
	//! connect signals and slots of all dialog controls
	void connectSignals();
	void setupWeigthTable();
	void setupProximityBox();
	void setupDistanceBox();
	//TODO check if the weights sum up to 100%
	void checkWeightValues();

	QList<csvFileData>* m_data;
	std::vector<double>* m_weights;
	iAMultidimensionalScaling* m_mds;

	QButtonGroup* m_proxiGroup;
	QButtonGroup* m_disGroup;

	
};