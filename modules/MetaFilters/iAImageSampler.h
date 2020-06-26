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
class iAModalityList;
class iASamplingResults;
class iASingleResult;
class iADerivedOutputCalculator;
class iACommandRunner;

class MetaFilters_API iAImageSampler: public QThread, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:
	iAImageSampler(
		QSharedPointer<iAModalityList const> modalities,
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
		int samplingID);
	QSharedPointer<iASamplingResults> results();
	void run() override;
	double elapsed() const override;
	double estimatedTimeRemaining() const override;
	void abort() override;
	bool isAborted();
signals:
	void Progress(int);
	void Status(QString const &);
private:
	//! @{
	//! input
	QSharedPointer<iAModalityList const> m_modalities;
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

	int m_curLoop;
	bool m_aborted;

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
	QMutex m_mutex;
	int m_runningOperations;
	int m_parameterCount;
	int m_samplingID;

	void StatusMsg(QString const & msg);
private slots:
	void computationFinished();
	void derivedOutputFinished();
};
