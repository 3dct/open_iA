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

#include <iAAttributes.h>
#include <iAPerformanceHelper.h>

#include <QMap>
#include <QMutex>
#include <QSharedPointer>
#include <QThread>

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
		QMap<QString, QVariant> const & parameters,
		QSharedPointer<iAAttributes> parameterRanges,
		QSharedPointer<iAParameterGenerator> sampleGenerator,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
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
	QMap<QString, QVariant> const& m_parameters;
	QSharedPointer<iAAttributes> m_parameterRanges;
	QSharedPointer<iAParameterGenerator> m_sampleGenerator;
	QString m_parameterRangeFile;
	QString m_parameterSetFile;
	QString m_derivedOutputFile;
	//! @}

	ParameterSetsPointer m_parameterSets;
	int m_curSample;
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
