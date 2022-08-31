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

#include "MetaFilters_export.h"

#include "iADurationEstimator.h"
#include "iASamplingMethod.h"

#include <iAAbortListener.h>
#include <iAAttributes.h>
#include <iAPerformanceHelper.h>

#include <QMap>
#include <QSharedPointer>
#include <QThread>

class iADerivedOutputCalculator;
class iASampleOperation;
class iASamplingResults;
class iASingleResult;

class iADataSet;
class iALogger;
class iAProgress;

class MetaFilters_API iAImageSampler: public QObject, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:
	iAImageSampler(
		std::vector<std::shared_ptr<iADataSet>> datasets,
		QVariantMap const & parameters,
		QSharedPointer<iAAttributes> parameterRanges,
		QSharedPointer<iAAttributes> parameterSpecs,
		QSharedPointer<iASamplingMethod> samplingMethod,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		int samplingID,
		iALogger * logger,
		iAProgress * progress);
	QSharedPointer<iASamplingResults> results();
	void start();
	double elapsed() const override;
	double estimatedTimeRemaining(double percent) const override;
	void abort() override;
	bool isAborted();
signals:
	void finished();
private:
	//! @{
	//! input
	std::vector<std::shared_ptr<iADataSet>> m_datasets;
	QVariantMap const& m_parameters;
	QSharedPointer<iAAttributes> m_parameterRanges;
	QSharedPointer<iAAttributes> m_parameterSpecs;
	QSharedPointer<iASamplingMethod> m_samplingMethod;
	QString m_parameterRangeFile;
	QString m_parameterSetFile;
	QString m_derivedOutputFile;
	//! @}

	iAParameterSetsPointer m_parameterSets;
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
	QMap<iASampleOperation*, int > m_runningComputation;
	QMap<iADerivedOutputCalculator*, QSharedPointer<iASingleResult> > m_runningDerivedOutput;

	QSharedPointer<iASamplingResults> m_results;
	int m_parameterCount;
	int m_samplingID;
	
	QStringList m_additionalArgumentList;
	int m_numDigits;
	iALogger* m_logger;
	iAProgress* m_progress;

	void newSamplingRun();
	void statusMsg(QString const & msg);
private slots:
	void computationFinished();
	void derivedOutputFinished();
};
