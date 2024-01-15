// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_FeatureAnalyzer.h"

#include <iASavableProject.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <QFileInfo>
#include <QMap>
#include <QTableWidget>


typedef iAQTtoUIConnector<QMainWindow, Ui_FeatureAnalyzer> FeatureAnalyzerConnector;

class iAPDMView;
class iAPreviewSPLOMView;
class iASegm3DView;
struct iASelection;
class iASelectionsView;
class iASPMView;
class iASSView;
class iATreeView;

class iAMainWindow;

class vtkIdTypeArray;

class QButtonGroup;
class QTreeWidgetItem;

class iAFeatureAnalyzer : public FeatureAnalyzerConnector, public iASavableProject
{
	Q_OBJECT

public:
	iAFeatureAnalyzer(iAMainWindow* mWnd, const QString& resDir, const QString& datasetsDir, QWidget* parent = nullptr);
	~iAFeatureAnalyzer();
	void LoadStateAndShow();

signals:
	void loadTreeDataToViews();
	void loadOverviewSelectionToSPM( QModelIndexList indices );
	void runsOffsetChanged( int );

private slots:
	void ShowSelections( bool checked );
	void ShowTreeView( bool checked );
	void selectionLoaded( iASelection * sel );
	void tabChanged( int index );

private:
	void LoadData();
	void AddSubdirectory(const QString& subDirName);
	void ParseComputerCSV(const QFileInfo& fi);
	void CalculateRunsOffset();
	//void GenerateMasksData();
	bool doSaveProject(QString const& projectFileName) override;

	QTableWidget m_data;
	QTableWidget m_referenceData;
	QMap<QString, double> m_gtPorosityMap;
	QString m_dataDir;
	QString m_datasetsDir;
	iASPMView * m_spmView;
	iATreeView * m_treeView;
	iAPDMView * m_pdmView;
	//iAPCView * m_pcView;
	iASSView * m_ssView;
	iASelectionsView * m_selView;
	iASegm3DView * m_segm3DView;
	iAPreviewSPLOMView * m_prvSplomView;
	int m_runsOffset;
	QMainWindow *m_visanMW;
};
