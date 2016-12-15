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
#include "pch.h"
#include "iAImageSampler.h"

#include "iAConsole.h"
#include "iACommandRunner.h"
#include "iADerivedOutputCalculator.h"
#include "iAAttributes.h"
#include "iAAttributeDescriptor.h"
#include "iAImageCoordinate.h"
#include "iAParameterGenerator.h"
#include "iASingleResult.h"
#include "iAStringHelper.h"
#include "iASamplingResults.h"
#include "iAModality.h"

#include <QDir>
#include <QMap>
#include <QTextStream>

const int CONCURRENT_COMPUTATION_RUNS = 1;

iAPerformanceTimer m_computationTimer;

iAImageSampler::iAImageSampler(
		QSharedPointer<iAModalityList const> modalities,
		QSharedPointer<iAAttributes> parameters,
		QSharedPointer<iAParameterGenerator> sampleGenerator,
		int sampleCount,
		int labelCount,
		QString const & outputBaseDir,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		QString const & computationExecutable,
		QString const & additionalArguments,
		QString const & pipelineName) :
	m_modalities(modalities),
	m_parameters(parameters),
	m_sampleGenerator(sampleGenerator),
	m_sampleCount(sampleCount),
	m_labelCount(labelCount),
	m_curLoop(0),
	m_parameterSets(0),
	m_executable(computationExecutable),
	m_additionalArguments(additionalArguments),
	m_pipelineName(pipelineName),
	m_outputBaseDir(outputBaseDir),
	m_aborted(false),
	m_parameterRangeFile(parameterRangeFile),
	m_parameterSetFile  (parameterSetFile),
	m_derivedOutputFile (derivedOutputFile),
	m_runningOperations(0),
	m_computationDuration(0),
	m_derivedOutputDuration(0)
{
}

void iAImageSampler::StatusMsg(QString const & msg)
{
	QString statusMsg(msg);
	if (statusMsg.length() > 105);
	statusMsg = statusMsg.left(100) + "...";
	emit Status(statusMsg);
	DEBUG_LOG(msg);
}

void iAImageSampler::run()
{
	m_overallTimer.start();
	StatusMsg("Generating sampling parameter sets...");
	m_parameterSets = m_sampleGenerator->GetParameterSets(m_parameters, m_sampleCount);
	if (!m_parameterSets)
	{
		DEBUG_LOG("No Parameters available!");
		return;
	}

	m_parameterCount = m_parameters->size();

	QStringList additionalArgumentList = SplitPossiblyQuotedString(m_additionalArguments);

	// add derived output to the attributes (which we want to set during sampling):
	QSharedPointer<iAAttributeDescriptor> objectCountAttr(new iAAttributeDescriptor(
		"Object Count", iAAttributeDescriptor::DerivedOutput, Discrete));
	QSharedPointer<iAAttributeDescriptor> avgUncertaintyAttr(new iAAttributeDescriptor(
		"Average Uncertainty", iAAttributeDescriptor::DerivedOutput, Continuous));
	QSharedPointer<iAAttributeDescriptor> timeAttr(new iAAttributeDescriptor(
		"Performance", iAAttributeDescriptor::DerivedOutput, Continuous));
	m_parameters->Add(objectCountAttr);
	m_parameters->Add(avgUncertaintyAttr);
	m_parameters->Add(timeAttr);

	m_results = QSharedPointer<iASamplingResults>(new iASamplingResults(
		m_parameters,
		m_sampleGenerator->GetName(),
		m_outputBaseDir,
		m_executable,
		m_additionalArguments,
		m_pipelineName,
		iASamplingResults::GetNewID()));

	for (m_curLoop=0; !m_aborted && m_curLoop<m_parameterSets->size(); ++m_curLoop)
	{
		ParameterSet const & paramSet = m_parameterSets->at(m_curLoop);
		while (m_runningComputation.size() >= CONCURRENT_COMPUTATION_RUNS && !m_aborted)
		{
			QThread::msleep(100);
		}
		if (m_aborted)
		{
			break;
		}
		StatusMsg(QString("Sampling run %1:").arg(m_curLoop));
		QString outputDirectory = m_outputBaseDir + "/sample" + QString::number(m_curLoop);
		QDir d(QDir::root());
		if (!d.mkpath(outputDirectory))
		{
			DEBUG_LOG(QString("Could not create output directory '%1'").arg(outputDirectory));
			return;
		}
		QString outputFile = outputDirectory + "/label.mhd";
		QStringList argumentList;
		argumentList << additionalArgumentList;
		argumentList << outputFile;

		for (int i = 0; i < m_modalities->size(); ++i)
		{
			argumentList << QString("%1").arg(m_modalities->Get(i)->GetFileName()) ;
		}

		for (int i = 0; i < m_parameterCount; ++i)
		{
			QString value;
			switch (m_parameters->at(i)->GetValueType())
			{
			case Continuous:
				value = QString::number(paramSet.at(i), 'g', 12);
				break;
			case Discrete:
				value = QString::number(static_cast<long>(paramSet.at(i)));
				break;
			case Categorical:
				// TODO: think about writing actual category names instead of indices as params?
				value = QString::number(static_cast<long>(paramSet.at(i)));
				break;
			}
			argumentList << value;
		}
		iACommandRunner* cmd = new iACommandRunner(m_executable, argumentList);
		
		QSharedPointer<iAModality const> mod0 = m_modalities->Get(0);
		
		m_runningComputation.insert(cmd, m_curLoop);
		connect(cmd, SIGNAL(finished()), this, SLOT(computationFinished()) );
		
		m_mutex.lock();
		m_runningOperations++;
		m_mutex.unlock();
		cmd->start();
	}
	if (m_aborted)
	{
		StatusMsg("Aborted by user!");
		return;
	}
	// wait for running operations to finish:
	while (m_runningOperations > 0)
	{
		msleep(100);
	}
}

void iAImageSampler::computationFinished()
{
	iACommandRunner* cmd = dynamic_cast<iACommandRunner*>(QObject::sender());
	if (!cmd)
	{
		DEBUG_LOG("Invalid state: NULL sender in computationFinished!");
		return;
	}
	int id = m_runningComputation[cmd];
	iAPerformanceTimer::DurationType computationTime = cmd->duration();
	StatusMsg(QString("Sampling run %1: Finished in %2 seconds; output: %3")
		.arg(QString::number(id))
		.arg(QString::number(computationTime))
		.arg(cmd->output()));
	m_computationDuration += computationTime;
	if (!cmd->success())
	{
		DEBUG_LOG(QString("Computation was NOT successful!"));
		m_aborted = true;

		// we don't start derived output calculation (at which's end we would do this otherwise):
		m_mutex.lock();
		m_runningOperations--;
		m_mutex.unlock();
		return;
	}
	ParameterSet const & param = m_parameterSets->at(id);

	QSharedPointer<iASingleResult> result = iASingleResult::Create(id, *m_results.data(), param);
	
	result->SetAttribute(m_parameterCount+2, computationTime);
	m_results->GetAttributes()->at(m_parameterCount+2)->AdjustMinMax(computationTime);

	// TODO: use external programs to calculate derived output!
	iADerivedOutputCalculator * newCharCalc = new iADerivedOutputCalculator (result, m_parameterCount, m_parameterCount+1, m_labelCount);
	m_runningDerivedOutput.insert(newCharCalc, result);
	connect(newCharCalc, SIGNAL(finished()), this, SLOT(derivedOutputFinished()) );
	newCharCalc->start();

	m_runningComputation.remove(cmd);
	delete cmd;

}


void iAImageSampler::derivedOutputFinished()
{
	iADerivedOutputCalculator* charactCalc = dynamic_cast<iADerivedOutputCalculator*>(QObject::sender());
	if (!charactCalc || !charactCalc->success())
	{
		DEBUG_LOG("ERROR: Derived output calculation was not successful! Possible reasons include that sampling did not produce a result,"
			" or that the result did not have the expected data type '(signed) integer'.");
		m_mutex.lock();
		m_runningOperations--;
		m_mutex.unlock();
		m_runningDerivedOutput.remove(charactCalc);
		delete charactCalc;
		return;
	}

	QSharedPointer<iASingleResult> result = m_runningDerivedOutput[charactCalc];
	m_results->GetAttributes()->at(m_parameterCount)->AdjustMinMax(result->GetAttribute(m_parameterCount));
	m_results->GetAttributes()->at(m_parameterCount+1)->AdjustMinMax(result->GetAttribute(m_parameterCount+1));

	// TODO: pass in from somewhere! Or don't store here at all? but what in case of a power outage/error?
	QString sampleMetaFile      = m_outputBaseDir + "/" + m_parameterRangeFile;
	QString parameterSetFile    = m_outputBaseDir + "/" + m_parameterSetFile;
	QString derivedOutputFile = m_outputBaseDir + "/" + m_derivedOutputFile;
	m_results->AddResult(result);
	emit Progress((100*m_results->size()) / m_parameterSets->size());
	if (!m_results->Store(sampleMetaFile, parameterSetFile, derivedOutputFile))
	{
		DEBUG_LOG("Error writing parameter file.");
	}
	m_runningDerivedOutput.remove(charactCalc);
	delete charactCalc;
	m_mutex.lock();
	m_runningOperations--;
	m_mutex.unlock();
}

double iAImageSampler::elapsed() const
{
	return m_overallTimer.elapsed();
}

double iAImageSampler::estimatedTimeRemaining() const
{
	return 
		(m_overallTimer.elapsed()/(m_curLoop+1)) // average duration of one cycle
		* static_cast<double>(m_parameterSets->size()-m_curLoop-1) // remaining cycles
	;
}

QSharedPointer<iASamplingResults> iAImageSampler::GetResults()
{
	return m_results;
}

void iAImageSampler::Abort()
{
	m_aborted = true;
}

bool iAImageSampler::IsAborted()
{
	return m_aborted;
}
