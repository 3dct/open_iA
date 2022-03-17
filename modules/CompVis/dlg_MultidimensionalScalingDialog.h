/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
	dlg_MultidimensionalScalingDialog(QWidget* parent, QList<csvFileData>* data, iAMultidimensionalScaling* mds);

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