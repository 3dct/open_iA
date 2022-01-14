/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

	iACompVisMain* m_main;
	QList<csvFileData>* m_data;
	iAMultidimensionalScaling* m_mds;
	dlg_MultidimensionalScalingDialog* m_MDSD;
	

   private slots:
};
