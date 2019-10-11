/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <iAModuleAttachmentToChild.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class dlg_GEMSe;
class dlg_GEMSeControl;
class dlg_labels;
class dlg_priors;
class dlg_samplings;
class iAWidgetAddHelper;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class QSettings;

class iAGEMSeAttachment : public iAModuleAttachmentToChild
{
	Q_OBJECT
public:
	static iAGEMSeAttachment* create(MainWindow * mainWnd, MdiChild * child);
	bool loadSampling(QString const & smpFileName, int labelCount, int datasetID);
	bool loadClustering(QString const & fileName);
	bool loadRefImg(QString const & refImgName);
	void setSerializedHiddenCharts(QString const & hiddenCharts);
	void setLabelInfo(QString const & colorTheme, QString const & labelNames);
	void saveProject(QSettings & metaFile, QString const & fileName);

	void resetFilter();
	void toggleAutoShrink();
	void toggleDockWidgetTitleBar();
	void exportClusterIDs();
	void exportAttributeRangeRanking();
	void exportRankings();
	void importRankings();
private:
	iAGEMSeAttachment(MainWindow * mainWnd, MdiChild * child);
	dlg_labels*						  m_dlgLabels;
	dlg_priors*                       m_dlgPriors;
	dlg_GEMSeControl*                 m_dlgGEMSeControl;
	QWidget*                          m_dummyTitleWidget;
	dlg_GEMSe*                        m_dlgGEMSe;
	dlg_samplings*                    m_dlgSamplings;
	QSharedPointer<iAWidgetAddHelper> m_widgetAddHelper;
};
