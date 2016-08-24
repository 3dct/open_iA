/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include <QMap>
#include <QMutex>
#include <QSharedPointer>
#include <QThread>

#include "iAAbortListener.h"
#include "iADurationEstimator.h"
#include "iAParameterGenerator.h"
#include "iAPerformanceHelper.h"
#include "iASpectrumType.h"

class iAAttributes;
class iAModalityList;
class iASamplingResults;
class iASingleResult;
class CharacteristicsCalculator;
class iACommandRunner;

class iAImageSampler: public QThread, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:
	iAImageSampler(
		QSharedPointer<iAModalityList const> modalities,
		QSharedPointer<iAAttributes> range,
		QSharedPointer<iAParameterGenerator> sampleGenerator,
		int sampleCount,
		QString const & outputBaseDir,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & characteristicsFile,
		QString const & computationExecutable,
		QString const & additionalArguments);
	QSharedPointer<iASamplingResults> GetResults();
	void run();
	virtual double elapsed() const;
	virtual double estimatedTimeRemaining() const;
	void Abort();
	bool IsAborted();
signals:
	void Progress(int);
	void Status(QString const &);
private:
	//! @{
	//! input
	QSharedPointer<iAModalityList const> m_modalities;
	QSharedPointer<iAAttributes> m_parameters;
	QSharedPointer<iAParameterGenerator> m_sampleGenerator;
	int m_sampleCount;
	ParameterSetsPointer m_parameterSets;
	QString m_computationExecutable;
	QString m_additionalArguments;
	QString m_outputBaseDir;

	QString m_parameterRangeFile;
	QString m_parameterSetFile;
	QString m_characteristicsFile;
	//! @}

	size_t m_curLoop;
	bool m_aborted;

	//! @{
	//! Performance Measurement
	iAPerformanceTimer m_overallTimer;
	iAPerformanceTimer::DurationType m_computationDuration;
	iAPerformanceTimer::DurationType m_derivedOutputDuration;
	//! @}

	// intention: running several extended random walker's in parallel
	// downside: seems to slow down rather than speed up overall process
	QMap<iACommandRunner*, int > m_runningComputation;
	QMap<CharacteristicsCalculator*, QSharedPointer<iASingleResult> > m_runningDerivedOutput;

	QSharedPointer<iASamplingResults> m_results;
	QMutex m_mutex;
	int m_runningOperations;
	int m_parameterCount;

	void StatusMsg(QString const & msg);
private slots:
	void computationFinished();
	void derivedOutputFinished();
};
