/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "MetaFilters_export.h"

#include "iAAbortListener.h"
#include "iADurationEstimator.h"
#include "iAParameterGenerator.h"

#include <iAPerformanceHelper.h>

#include <QMap>
#include <QMutex>
#include <QSharedPointer>
#include <QThread>

class iAAttributes;
class iASamplingResults;
class iASingleResult;
class iADerivedOutputCalculator;
class iACommandRunner;

class MetaFilters_API iAImageSampler: public QObject, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:
	iAImageSampler(
		QStringList fileNames,
		QSharedPointer<iAAttributes> parameterRanges,
		QSharedPointer<iAParameterGenerator> sampleGenerator,
		int sampleCount,
		int labelCount,
		QString const & outputBaseDir,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		QString const & computationExecutable,
		QString const & additionalArguments,
		QString const & pipelineName,
		QString const & imageBaseName,
		bool separateOutputDir,
		bool calculateChar,
		bool abortOnError,
		int samplingID);
	QSharedPointer<iASamplingResults> results();
	void start();
	double elapsed() const override;
	double estimatedTimeRemaining() const override;
	void abort() override;
	bool isAborted();
signals:
	void progress(int);
	void status(QString const &);
	void finished();
private:
	//! @{
	//! input
	QStringList m_fileNames;
	QSharedPointer<iAAttributes> m_parameterRanges;
	QSharedPointer<iAParameterGenerator> m_sampleGenerator;
	int m_sampleCount;
	int m_labelCount;
	ParameterSetsPointer m_parameterSets;
	QString m_executable;
	QString m_additionalArguments;
	QString m_outputBaseDir;
	QString m_pipelineName;

	QString m_parameterRangeFile;
	QString m_parameterSetFile;
	QString m_derivedOutputFile;

	QString m_imageBaseName;
	bool m_separateOutputDir;
	bool m_calculateCharacteristics;
	//! @}

	int m_curSample;
	bool m_aborted;
	//! set to true if sampling should be aborted if an error is encountered,
	//! set to false to continue sampling with next parameter set
	bool m_abortOnError;

	//! @{
	//! Performance Measurement
	iAPerformanceTimer m_overallTimer;
	iAPerformanceTimer::DurationType m_computationDuration;
	iAPerformanceTimer::DurationType m_derivedOutputDuration;
	//! @}

	// intention: running several sampled algorithms in parallel
	// downside: seems to slow down rather than speed up overall process
	QMap<iACommandRunner*, int > m_runningComputation;
	QMap<iADerivedOutputCalculator*, QSharedPointer<iASingleResult> > m_runningDerivedOutput;

	QSharedPointer<iASamplingResults> m_results;
	int m_parameterCount;
	int m_samplingID;
	
	QStringList m_additionalArgumentList;
	int m_numDigits;

	void newSamplingRun();
	void statusMsg(QString const & msg);
private slots:
	void computationFinished();
	void derivedOutputFinished();
};
