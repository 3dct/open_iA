/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#ifndef iARunBatchThread_h__
#define iARunBatchThread_h__

#include <QList>
#include <QString>
#include <QTableWidget>
#include <QThread>

#include "PorosityAnalyserHelpers.h"
#include "iACalculatePoreProperties.h"

class iAPorosityAnalyserModuleInterface;
struct RunInfo;

class iARunBatchThread : public QThread
{
	Q_OBJECT
public:
	iARunBatchThread( QObject * parent = 0 ) : QThread( parent ) {};
	void Init( iAPorosityAnalyserModuleInterface *pmi, QString datasetsDescriptionFile, bool rbNewPipelineDataNoPores,
			   bool rbNewPipelineData, bool rbExistingPipelineData, iACalculatePoreProperties * poreProps );
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
	void calculatePoreChars( QString masksCSVPath );

	iAPorosityAnalyserModuleInterface * m_pmi;
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
	bool m_rbExistingPipelineData;

	iACalculatePoreProperties* m_poreProps;

signals:
	void batchProgress( int progress );
	void totalProgress( int progress );
};

#endif // iARunBatchThread_h__
