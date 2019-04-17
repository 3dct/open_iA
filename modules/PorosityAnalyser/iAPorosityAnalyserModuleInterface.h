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

#include "ui_ComputeSegmentations.h"

#include <defines.h>
#include <iAModuleInterface.h>

class iAPorosityAnalyser;

class iAPorosityAnalyserModuleInterface : public iAModuleInterface
{
	Q_OBJECT

public:
	void Initialize() override;
	void SaveSettings() const override;
	Ui::ComputeSegmentations * ui();
	void log( QString text, bool appendToPrev = false );
	QString DatasetFolder() const;
	QString ResultsFolder() const;
	QString CSVFile() const;
	QString CpuVendor() const { return m_cpuVendor; }
	QString CpuBrand() const { return m_cpuBrand; }
	QString ComputerName() const;

private slots:
	void computeParameterSpace();
	void launchPorosityAnalyser();
	//void launchCalcPoreProps();
	void loadCSV();
	void saveCSV();
	void browseCSV();
	void browserResultsFolder();
	void browserDatasetsFolder();
	void runCalculations();
	void showHideLogs();
	void batchProgress(int progress);
	void totalProgress(int progress);
	void currentBatch( QString str );
	void clearPipeline();
	void addPipeline();
	void resizePipeline();
	void clearTableWidgetItem();
	void generateDatasetPreviews();
	void datasetPreviewThreadFinished();
	void datasetPreviewThreadStarted();
	void displayPipelineInSlots( QTableWidgetItem * );
	void compNameChanged();

protected:
	void updateFromGUI() const;
	void setupTableWidgetContextMenu();
	void createTableWidgetActions();
	
protected:
	QWidget * m_compSegmWidget;
	Ui::ComputeSegmentations uiComputeSegm;
	mutable QString m_computerName;
	mutable QString m_resultsFolder;
	mutable QString m_datasetsFolder;
	mutable QString m_csvFile;
	QString m_cpuVendor;
	QString m_cpuBrand;	
	iAPorosityAnalyser * m_porosityAnalyser;
	QAction *removeRowAction;
	QAction *saveTableToCSVAction;
	QAction *loadTableFromCSVAction;

private:
	int m_pipelineSlotsCount;
	QSize m_pipelineSlotIconSize;

	void removeGTDatasets( QStringList& list, const QStringList& toDelete );
};
