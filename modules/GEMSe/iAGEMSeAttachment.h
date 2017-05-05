/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAModuleAttachmentToChild.h"

#include <QSharedPointer>
#include <vtkSmartPointer.h>

class dlg_GEMSe;
class dlg_GEMSeControl;
class dlg_labels;
class dlg_priors;
class dlg_samplings;
class iAWidgetAddHelper;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class iAGEMSeAttachment : public iAModuleAttachmentToChild
{
	Q_OBJECT
public:
	static iAGEMSeAttachment* create(MainWindow * mainWnd, iAChildData childData);
	bool LoadSampling(QString const & smpFileName, int labelCount, int datasetID);
	bool LoadClustering(QString const & fileName);
	bool LoadRefImg(QString const & refImgName);
	void SetSerializedHiddenCharts(QString const & hiddenCharts);
	void SetLabelInfo(QString const & colorTheme, QString const & labelNames);

	void ResetFilter();
	void ToggleAutoShrink();
	void ToggleDockWidgetTitleBar();
	void ExportClusterIDs();
	void ExportAttributeRangeRanking();
	void ExportRankings();
	void ImportRankings();
private:
	iAGEMSeAttachment(MainWindow * mainWnd, iAChildData childData);
	dlg_labels*						  m_dlgLabels;
	dlg_priors*                       m_dlgPriors;
	dlg_GEMSeControl*                 m_dlgGEMSeControl;
	QWidget*                          m_dummyTitleWidget;
	dlg_GEMSe*                        m_dlgGEMSe;
	dlg_samplings*                    m_dlgSamplings;
	QSharedPointer<iAWidgetAddHelper> m_widgetAddHelper;
};
