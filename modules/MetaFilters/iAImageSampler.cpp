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
#include "iAImageSampler.h"

#include "iAAttributes.h"
#include "iACommandRunner.h"
#include "iADerivedOutputCalculator.h"
#include "iAParameterGenerator.h"
#include "iASingleResult.h"
#include "iASampleParameterNames.h"
#include "iASamplingResults.h"

#include <iAAttributeDescriptor.h>
#include <iAConsole.h>
#include <iAImageCoordinate.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iANameMapper.h>
#include <iAStringHelper.h>

#include <QDir>
#include <QMap>
#include <QTextStream>

const int CONCURRENT_COMPUTATION_RUNS = 1;

iAPerformanceTimer m_computationTimer;

iAImageSampler::iAImageSampler(
		QStringList fileNames,
		QMap<QString, QVariant> const & parameters,
		QSharedPointer<iAAttributes> parameterRanges,
		QSharedPointer<iAParameterGenerator> sampleGenerator,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		int samplingID) :
	m_fileNames(fileNames),
	m_parameters(parameters),
	m_parameterRanges(parameterRanges),
	m_sampleGenerator(sampleGenerator),
	m_parameterRangeFile(parameterRangeFile),
	m_parameterSetFile  (parameterSetFile),
	m_derivedOutputFile (derivedOutputFile),
	m_curSample(0),
	m_aborted(false),
	m_computationDuration(0),
	m_derivedOutputDuration(0),
	m_samplingID(samplingID)
{
}

void iAImageSampler::statusMsg(QString const & msg)
{
	QString statusMsg(msg);
	if (statusMsg.length() > 105)
	{
		statusMsg = statusMsg.left(100) + "...";
	}
	emit status(statusMsg);
	DEBUG_LOG(msg);
}

void iAImageSampler::newSamplingRun()
{
	if (m_aborted)
	{
		statusMsg("----------SAMPLING ABORTED!----------");
		return;
	}
	if (m_curSample >= m_parameterSets->size())
	{
		if (m_runningComputation.size() == 0)
		{
			statusMsg("---------- SAMPLING FINISHED! ----------");
			emit finished();
		}
		return;
	}
	if (m_runningComputation.size() > CONCURRENT_COMPUTATION_RUNS)
	{
		statusMsg(QString("Tried to start sampling run %1 when still %2 computations running!")
			.arg(m_curSample).arg(m_runningComputation.size()));
	}
	statusMsg(QString("Sampling run %1.").arg(m_curSample));
	ParameterSet const& paramSet = m_parameterSets->at(m_curSample);
	QString outputDirectory(m_parameters[spnOutputFolder].toString());
	if (m_parameters[spnSubfolderPerSample].toBool())
	{
		outputDirectory = outputDirectory + "/sample" + QString::number(m_curSample);
	}
	QDir d(QDir::root());
	if (!QDir(outputDirectory).exists() && !d.mkpath(outputDirectory))
	{
		statusMsg(QString("Could not create output directory '%1'").arg(outputDirectory));
		return;
	}
	QFileInfo fi(m_parameters[spnBaseName].toString());
	QString outputFile = outputDirectory + "/" + (m_parameters[spnSubfolderPerSample].toBool() ?
		m_parameters[spnBaseName].toString() :
		QString("%1%2%3").arg(fi.baseName()).arg(m_curSample, m_numDigits, 10, QChar('0')).arg(
			fi.completeSuffix().size() > 0 ? QString(".%1").arg(fi.completeSuffix()) : QString(""))
		);

	if (m_parameters[spnAlgorithmType].toString() == atBuiltIn)
	{
		// TODO
	}
	else if (m_parameters[spnAlgorithmType].toString() == atExternal)
	{
		QStringList argumentList;
		argumentList << m_additionalArgumentList;
		argumentList << outputFile;

		for (QString fileName : m_fileNames)
		{
			argumentList << fileName;
		}

		for (int i = 0; i < m_parameterCount; ++i)
		{
			QString value;
			switch (m_parameterRanges->at(i)->valueType())
			{
			default:
			case Continuous:
				value = QString::number(paramSet.at(i), 'g', 12);
				break;
			case Discrete:
				value = QString::number(static_cast<long>(paramSet.at(i)));
				break;
			case Categorical:
				value = m_parameterRanges->at(i)->nameMapper()->name(static_cast<long>(paramSet.at(i)));
				break;
			}
			argumentList << value;
		}
		iACommandRunner* cmd = new iACommandRunner(m_parameters[spnExecutable].toString(), argumentList);

		m_runningComputation.insert(cmd, m_curSample);
		connect(cmd, &iACommandRunner::finished, this, &iAImageSampler::computationFinished);
		cmd->start();
	}
	++m_curSample;
}

void iAImageSampler::start()
{
	m_overallTimer.start();
	if (m_parameterRanges->size() == 0)
	{
		statusMsg("Algorithm has no parameters, nothing to sample!");
		return;
	}
	if (m_parameters[spnAlgorithmType].toString() != atBuiltIn &&
		m_parameters[spnAlgorithmType].toString() != atExternal)
	{
		statusMsg(QString("Unknown algorithm type '%1'").arg(m_parameters[spnAlgorithmType].toString()));
		return;
	}
	if (m_parameters[spnAlgorithmType].toString() == atExternal && !QFile(m_parameters[spnExecutable].toString()).exists())
	{
		statusMsg(QString("Executable '%1' doesn't exist!").arg(m_parameters[spnExecutable].toString()));
		return;
	}
	if (m_fileNames.size() == 0)
	{
		statusMsg("No input given!");
		return;
	}
	statusMsg("");
	statusMsg("---------- SAMPLING STARTED ----------");
	statusMsg("Generating sampling parameter sets...");
	m_parameterSets = m_sampleGenerator->GetParameterSets(m_parameterRanges, m_parameters[spnNumberOfSamples].toInt());
	if (!m_parameterSets)
	{
		statusMsg("No Parameters available!");
		return;
	}
	m_parameterCount = countAttributes(*m_parameterRanges.data(), iAAttributeDescriptor::Parameter);

	m_additionalArgumentList = splitPossiblyQuotedString(m_parameters[spnAdditionalArguments].toString());
	if (findAttribute(*m_parameterRanges.data(), "Object Count") == -1)
	{
		// add derived output to the attributes (which we want to set during sampling):
		QSharedPointer<iAAttributeDescriptor> objectCountAttr(new iAAttributeDescriptor(
			"Object Count", iAAttributeDescriptor::DerivedOutput, Discrete));
		QSharedPointer<iAAttributeDescriptor> avgUncertaintyAttr(new iAAttributeDescriptor(
			"Average Uncertainty", iAAttributeDescriptor::DerivedOutput, Continuous));
		QSharedPointer<iAAttributeDescriptor> timeAttr(new iAAttributeDescriptor(
			"Performance", iAAttributeDescriptor::DerivedOutput, Continuous));
		m_parameterRanges->push_back(objectCountAttr);
		m_parameterRanges->push_back(avgUncertaintyAttr);
		m_parameterRanges->push_back(timeAttr);
	}

	m_results = QSharedPointer<iASamplingResults>(new iASamplingResults(
		m_parameterRanges,
		m_sampleGenerator->name(),
		m_parameters[spnOutputFolder].toString(),
		m_parameters[spnExecutable].toString(),
		m_parameters[spnAdditionalArguments].toString(),
		m_parameters[spnAlgorithmName].toString(),
		m_samplingID));

	m_numDigits = std::floor(std::log10(std::abs(m_parameterSets->size()))) + 1;  // number of required digits for number >= 1
	for (int i = 0; i < CONCURRENT_COMPUTATION_RUNS; ++i)
	{
		newSamplingRun();
	}
}

void iAImageSampler::computationFinished()
{
	iACommandRunner* cmd = dynamic_cast<iACommandRunner*>(QObject::sender());
	if (!cmd)
	{
		statusMsg("Invalid state: nullptr sender in computationFinished!");
		return;
	}
	int id = m_runningComputation[cmd];
	m_runningComputation.remove(cmd);
	iAPerformanceTimer::DurationType computationTime = cmd->duration();
	statusMsg(QString("Finished in %1 seconds. Output: %2\n")
		.arg(QString::number(computationTime))
		.arg(cmd->output()));
	m_computationDuration += computationTime;
	if (!cmd->success())
	{
		statusMsg("Computation was NOT successful.");
		if (m_parameters[spnAbortOnError].toBool())
		{
			statusMsg("Aborting, since the user requested to abort on errors.");
			m_aborted = true;
			if (m_runningComputation.size() == 0)
			{
				emit finished();
			}
			return;
		}
	}
	ParameterSet const & param = m_parameterSets->at(id);

	// TODO: check/change: the filename here should probably match outputFile from newSamplingRun, or be removed?
	QSharedPointer<iASingleResult> result = iASingleResult::create(id, *m_results.data(), param,
		m_parameters[spnOutputFolder].toString() + "/sample" + QString::number(id) + +"/label.mhd");

	result->setAttribute(m_parameterCount+2, computationTime);
	m_results->attributes()->at(m_parameterCount+2)->adjustMinMax(computationTime);

	if (m_parameters[spnComputeDerivedOutput].toBool())
	{
		// TODO: use external programs / built-in filters to calculate derived output
		iADerivedOutputCalculator * newCharCalc = new iADerivedOutputCalculator(result, m_parameterCount, m_parameterCount + 1,
			m_parameters[spnNumberOfLabels].toInt());
		m_runningDerivedOutput.insert(newCharCalc, result);
		connect(newCharCalc, &iADerivedOutputCalculator::finished, this, &iAImageSampler::derivedOutputFinished);
		newCharCalc->start();
	}
	else
	{
		QString sampleMetaFile = m_parameters[spnOutputFolder].toString() + "/" + m_parameterRangeFile;
		QString parameterSetFile = m_parameters[spnOutputFolder].toString() + "/" + m_parameterSetFile;
		QString derivedOutputFile = m_parameters[spnOutputFolder].toString() + "/" + m_derivedOutputFile;
		m_results->addResult(result);
		emit progress((100 * m_results->size()) / m_parameterSets->size());
		if (!m_results->store(sampleMetaFile, parameterSetFile, derivedOutputFile))
		{
			statusMsg("Error writing parameter file.");
		}
	}
	delete cmd;
	newSamplingRun();
}


void iAImageSampler::derivedOutputFinished()
{
	iADerivedOutputCalculator* charactCalc = dynamic_cast<iADerivedOutputCalculator*>(QObject::sender());
	if (!charactCalc || !charactCalc->success())
	{
		statusMsg("ERROR: Derived output calculation was not successful! Possible reasons include that sampling did not produce a result,"
			" or that the result did not have the expected data type '(signed) integer'.");
		m_runningDerivedOutput.remove(charactCalc);
		delete charactCalc;
		newSamplingRun();
		return;
	}

	QSharedPointer<iASingleResult> result = m_runningDerivedOutput[charactCalc];
	m_results->attributes()->at(m_parameterCount)->adjustMinMax(result->attribute(m_parameterCount));
	m_results->attributes()->at(m_parameterCount+1)->adjustMinMax(result->attribute(m_parameterCount+1));

	// TODO: pass in from somewhere! Or don't store here at all? but what in case of a power outage/error?
	QString sampleMetaFile    = m_parameters[spnOutputFolder].toString() + "/" + m_parameterRangeFile;
	QString parameterSetFile  = m_parameters[spnOutputFolder].toString() + "/" + m_parameterSetFile;
	QString derivedOutputFile = m_parameters[spnOutputFolder].toString() + "/" + m_derivedOutputFile;
	m_results->addResult(result);
	emit progress((100*m_results->size()) / m_parameterSets->size());
	if (!m_results->store(sampleMetaFile, parameterSetFile, derivedOutputFile))
	{
		statusMsg("Error writing parameter file.");
	}
	m_runningDerivedOutput.remove(charactCalc);
	delete charactCalc;
	newSamplingRun();
}

double iAImageSampler::elapsed() const
{
	return m_overallTimer.elapsed();
}

double iAImageSampler::estimatedTimeRemaining() const
{
	return
		(m_overallTimer.elapsed()/(m_curSample +1)) // average duration of one cycle
		* static_cast<double>(m_parameterSets->size()- m_curSample -1) // remaining cycles
	;
}

QSharedPointer<iASamplingResults> iAImageSampler::results()
{
	return m_results;
}

void iAImageSampler::abort()
{
	statusMsg("Abort requested by User!");
	m_aborted = true;
}

bool iAImageSampler::isAborted()
{
	return m_aborted;
}
