// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

#include "iADurationEstimator.h"
#include "iASamplingMethod.h"

#include <iAAbortListener.h>
#include <iAAttributes.h>
#include <iAPerformanceHelper.h>

#include <QMap>
#include <QThread>

#include <memory>

class iADerivedOutputCalculator;
class iASampleOperation;
class iASamplingResults;
class iASingleResult;

class iADataSet;
class iAProgress;

class MetaFilters_API iAImageSampler: public QObject, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:
	iAImageSampler(
		std::map<size_t, std::shared_ptr<iADataSet>> datasets,
		QVariantMap const & parameters,
		std::shared_ptr<iAAttributes> parameterRanges,
		std::shared_ptr<iAAttributes> parameterSpecs,
		std::shared_ptr<iASamplingMethod> samplingMethod,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		int samplingID,
		iAProgress * progress);
	std::shared_ptr<iASamplingResults> results();
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
	std::map<size_t, std::shared_ptr<iADataSet>> m_dataSets;
	QVariantMap const& m_parameters;
	std::shared_ptr<iAAttributes> m_parameterRanges;
	std::shared_ptr<iAAttributes> m_parameterSpecs;
	std::shared_ptr<iASamplingMethod> m_samplingMethod;
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
	QMap<iADerivedOutputCalculator*, std::shared_ptr<iASingleResult> > m_runningDerivedOutput;

	std::shared_ptr<iASamplingResults> m_results;
	int m_parameterCount;
	int m_samplingID;
	
	QStringList m_additionalArgumentList;
	int m_numDigits;
	iAProgress* m_progress;

	void newSamplingRun();
	void statusMsg(QString const & msg);
private slots:
	void computationFinished();
	void derivedOutputFinished();
};
