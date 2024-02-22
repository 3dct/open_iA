// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_ComputeSegmentations.h"

#include <iAGUIModuleInterface.h>

class iAFeatureAnalyzerComputationModuleInterface : public iAGUIModuleInterface
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
	QAction *removeRowAction;
	QAction *saveTableToCSVAction;
	QAction *loadTableFromCSVAction;

private:
	int m_pipelineSlotsCount;
	QSize m_pipelineSlotIconSize;

	void removeGTDatasets( QStringList& list, const QStringList& toDelete );
};
