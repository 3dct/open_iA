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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA_MMSEG_SAMPLER_H
#define IA_MMSEG_SAMPLER_H

#include <QMap>
#include <QSharedPointer>
#include <QThread>

#include "iAAbortListener.h"
#include "iADurationEstimator.h"
#include "iAMMSegParameterListFwd.h"
#include "iAPerformanceHelper.h"
#include "iARandomWalker.h"
#include "iASpectrumType.h"
#include "SVMImageFilter.h"


class iAAttributes;
class iAModalityList;
class iAMMSegParameterRange;
class iAMMSegParameterGenerator;
class iASamplingResults;
class iASingleResult;
class CharacteristicsCalculator;

class iAMMSegSampler: public QThread, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:
	iAMMSegSampler(
		QSharedPointer<iAModalityList const> modalities,
		QSharedPointer<iAMMSegParameterRange> range,
		QSharedPointer<iAMMSegParameterGenerator> sampleGenerator,
		SVMImageFilter::SeedsPointer seeds,
		QString const & outputBaseDir,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & characteristicsFile,
		bool storeProbabilities);
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
	QSharedPointer<iAMMSegParameterRange> m_range;
	QSharedPointer<iAMMSegParameterGenerator> m_sampleGenerator;
	SVMImageFilter::SeedsPointer m_seeds;
	ParameterListPointer m_parameters;
	//! @}

	size_t m_curLoop;
	QString m_outputBaseDir;
	bool m_storeProbabilities;
	bool m_aborted;
	
	QString m_parameterRangeFile;
	QString m_parameterSetFile;
	QString m_characteristicsFile;

	//! @{
	//! Performance Measurement
	iAPerformanceTimer m_erwPerfTimer;
	iAPerformanceTimer m_ChaPerfTimer;
	iAPerformanceTimer::DurationType m_calcERWDuration;
	iAPerformanceTimer::DurationType m_calcChaDuration;
	//! @}

	// intention: running several extended random walker's in parallel
	// downside: seems to slow down rather than speed up overall process
	QMap<iAExtendedRandomWalker* , QSharedPointer<iAMMSegParameter> > m_runningERW;
	QMap<CharacteristicsCalculator*, QSharedPointer<iASingleResult> > m_runningCharCalc;

	QSharedPointer<iASamplingResults> m_results;
	QMutex m_mutex;
	int m_runningOperations;
	int m_derivedOutputStart;

	QSharedPointer<iAAttributes> m_attributes;

	void StatusMsg(QString const & msg);
private slots:
	void erwFinished();
	void charactCalcFinished();
};

#endif // IA_MMSEG_SAMPLER_H