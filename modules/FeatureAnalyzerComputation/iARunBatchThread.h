// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FeatureAnalyzerHelpers.h"

#include <QList>
#include <QString>
#include <QTableWidget>
#include <QThread>

class iAFeatureAnalyzerComputationModuleInterface;
struct RunInfo;

class iARunBatchThread : public QThread
{
	Q_OBJECT
public:
	iARunBatchThread( QObject * parent = 0 ) : QThread( parent ) {};
	void Init(iAFeatureAnalyzerComputationModuleInterface* pmi,
		QString datasetsDescriptionFile,
		bool rbNewPipelineDataNoPores,
		bool rbNewPipelineData);
protected:
	virtual void run();
	void executeNewBatches( QTableWidget & settingsCSV, QMap<int, bool> & isBatchNew );
	void executeBatch( const QList<PorosityFilterID> & filterIds, QString datasetName, QString batchDir, QTableWidget * settingsCSV, int row );
	void initRunsCSVFile( QTableWidget & runsCSV, QString batchDir, const QList<ParamNameType> & paramNames );
	void saveResultsToRunsCSV( RunInfo & results, QString masksDir, QTableWidget & runsCSV, bool success = true );
	void updateComputerCSVFile( QTableWidget & settingsCSV );
	void updateBatchesCSVFiles( QTableWidget & settingsCSV, QMap<int, bool> & isBatchNew );
	bool updateBatchesCSVFile( QTableWidget & settingsCSV, int row, QString batchesFile );
	void generateMasksCSVFile( QString batchDir, QString batchesDir );
	void calcFeatureCharsForMask(RunInfo &results, QString currMaskFilePath);

	iAFeatureAnalyzerComputationModuleInterface* m_pmi;
	QTableWidget m_runsCSV;
	QTableWidget m_settingsCSV;
	QTableWidget m_computerCSVData;
	QTableWidget m_batchesData;
	QTableWidget m_dsDescr;
	QTableWidget m_masksData;
	QMap<QString, QString> m_datasetGTs;
	QString m_datasetsDescrFile;
	bool m_rbNewPipelineDataNoPores;
	bool m_rbNewPipelineData;
signals:
	void batchProgress( int progress );
	void totalProgress( int progress );
	void currentBatch( QString str );
};
