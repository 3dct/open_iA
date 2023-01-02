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

#include <iATool.h>

#include <memory>

class dlg_GEMSe;
class dlg_GEMSeControl;
class dlg_priors;
class dlg_samplings;

class QSettings;
class QString;
class QWidget;

class iAGEMSeTool : public iATool
{
public:
	static std::shared_ptr<iATool> create(iAMainWindow* mainWnd, iAMdiChild* child);
	iAGEMSeTool(iAMainWindow* mainWnd, iAMdiChild* child);
	static const QString ID;
	void loadState(QSettings & projectFile, QString const & fileName) override;
	void saveState(QSettings & projectFile, QString const & fileName) override;

	bool loadSampling(QString const& smpFileName, int labelCount, int datasetID);
	bool loadClustering(QString const& fileName);
	bool loadRefImg(QString const& refImgName);
	void setSerializedHiddenCharts(QString const& hiddenCharts);
	void setLabelInfo(QString const& colorTheme, QString const& labelNames);
	void saveProject(QSettings& metaFile, QString const& fileName);

	void resetFilter();
	void toggleAutoShrink();
	void toggleDockWidgetTitleBar();
	void exportClusterIDs();
	void exportAttributeRangeRanking();
	void exportRankings();
	void importRankings();
private:
	dlg_priors* m_dlgPriors;
	dlg_GEMSeControl* m_dlgGEMSeControl;
	QWidget* m_dummyTitleWidget;
	dlg_GEMSe* m_dlgGEMSe;
	dlg_samplings* m_dlgSamplings;
};
