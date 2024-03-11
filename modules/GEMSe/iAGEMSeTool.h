// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <memory>

class dlg_GEMSe;
class dlg_GEMSeControl;
class dlg_samplings;

class iASEAFile;

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

	void resetFilter();
	void toggleAutoShrink();
	void toggleDockWidgetTitleBar();
	void exportClusterIDs();
	void exportAttributeRangeRanking();
	void exportRankings();
	void importRankings();
private:
	QWidget* m_dummyTitleWidget;
	dlg_samplings* m_dlgSamplings;
	dlg_GEMSe* m_dlgGEMSe;
	dlg_GEMSeControl* m_dlgGEMSeControl;
};
