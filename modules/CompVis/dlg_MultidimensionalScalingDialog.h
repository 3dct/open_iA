// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
//CompVis
#include "ui_MultidimensionalScalingDialog.h"
//iA
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
//Qt
#include <QDialog>
#include <QCheckBox>

class dlg_MultidimensionalScalingDialog : public QDialog, public Ui_MultidimensionalScalingDialog
{
	Q_OBJECT

   public:
	dlg_MultidimensionalScalingDialog(QList<csvFileData>* data, iAMultidimensionalScaling* mds);

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


	QList<csvFileData>* m_data;
	std::vector<double>* m_weights;
	iAMultidimensionalScaling* m_mds;

	QButtonGroup* m_proxiGroup;
	QButtonGroup* m_disGroup;


};
