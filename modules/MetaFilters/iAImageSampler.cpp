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
#include "iASampleBuiltInFilterOperation.h"
#include "iAParameterNames.h"
#include "iASamplingResults.h"

#include <iAAttributeDescriptor.h>
#include <iAConnector.h>
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


// TODO: find better place - declaration currently in iAParameterNames.h

#include <QFileInfo>

QString getOutputFolder(QString const& baseFolder, bool createSubFolder, int sampleNr, int numDigits)
{
	QString outputFolder(baseFolder);
	if (createSubFolder)
	{
		outputFolder = outputFolder + QString("/sample%1").arg(sampleNr, numDigits, 10, QChar('0'));
	}
	return outputFolder;
}

QString getOutputFileName(QString const& outputFolder, QString const& baseName,
	bool createSubFolder, int sampleNr, int numDigits)
{
	QFileInfo fi(baseName);
	return outputFolder + "/" + (createSubFolder ?
		baseName :
		QString("%1%2%3").arg(fi.completeBaseName()).arg(sampleNr, numDigits, 10, QChar('0')).arg(
			fi.suffix().size() > 0 ? QString(".%1").arg(fi.suffix()) : QString(""))
		);
}

int requiredDigits(int largestNumber)
{  // number of required digits for number >= 1
	return std::floor(std::log10(std::abs(largestNumber))) + 1;
}


iAPerformanceTimer m_computationTimer;

iAImageSampler::iAImageSampler(
		QSharedPointer<iAModalityList> dataset,
		QMap<QString, QVariant> const & parameters,
		QSharedPointer<iAAttributes> parameterRanges,
		QSharedPointer<iAParameterGenerator> sampleGenerator,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		int samplingID,
		iALogger * logger) :
	m_datasets(dataset),
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
	m_samplingID(samplingID),
	m_logger(logger)
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
		emit finished();
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
		return;
	}
	statusMsg(QString("Sampling run %1.").arg(m_curSample));
	iAParameterSet const& paramSet = m_parameterSets->at(m_curSample);
	QString outputFolder(getOutputFolder(
		m_parameters[spnOutputFolder].toString(),
		m_parameters[spnSubfolderPerSample].toBool(), m_curSample, m_numDigits));
	QDir d(QDir::root());
	if (!QDir(outputFolder).exists() && !d.mkpath(outputFolder))
	{
		statusMsg(QString("Could not create output folder '%1'").arg(outputFolder));
		return;
	}
	QString outputFile(getOutputFileName(outputFolder, m_parameters[spnBaseName].toString(),
		m_parameters[spnSubfolderPerSample].toBool(), m_curSample, m_numDigits));
	iASampleOperation* op(nullptr);

	if (m_parameters[spnAlgorithmType].toString() == atBuiltIn)
	{
		QMap<QString, QVariant> singleRunParams;
		for (int i = 0; i < m_parameterCount; ++i)
		{
			auto desc = m_parameterRanges->at(i);
			singleRunParams.insert(desc->name(), paramSet.at(i));
		}
		QVector<iAConnector*> input; // TODO - pass in...?
		QVector<QString> fileNames;
		for (int m = 0; m < m_datasets->size(); ++m)
		{
			auto con = new iAConnector();
			con->setImage(m_datasets->get(m)->image());
			input.push_back(con);
			fileNames.push_back(m_datasets->get(m)->fileName());
		}
		op = new iASampleBuiltInFilterOperation(
			m_parameters[spnFilter].toString(),
			m_parameters[spnCompressOutput].toBool(),
			m_parameters[spnOverwriteOutput].toBool(),
			singleRunParams, input, fileNames, outputFile, m_logger);
	}
	else if (m_parameters[spnAlgorithmType].toString() == atExternal)
	{
		QStringList argumentList;
		argumentList << m_additionalArgumentList;
		argumentList << outputFile;

		for (int m = 0; m < m_datasets->size(); ++m)
		{
			argumentList << m_datasets->get(m)->fileName();
		}

		for (int i = 0; i < m_parameterCount; ++i)
		{
			QString value;
			switch (m_parameterRanges->at(i)->valueType())
			{
			case Continuous:
				value = QString::number(paramSet.at(i).toDouble(), 'g', 12);
				break;
			case Discrete:
				value = QString::number(paramSet.at(i).toInt());
				break;
			case Categorical:
				value = m_parameterRanges->at(i)->nameMapper()->name(paramSet.at(i).toInt());
				break;
			default:
				value = paramSet.at(i).toString();
				break;
			}
			argumentList << value;
		}
		op = new iACommandRunner(m_parameters[spnExecutable].toString(), argumentList);
	}
	if (!op)
	{
		statusMsg("Invalid configuration - neither Built-in nor external sampling operation were created!");
		return;
	}
	m_runningComputation.insert(op, m_curSample);
	connect(op, &iASampleBuiltInFilterOperation::finished, this, &iAImageSampler::computationFinished);
	op->start();
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
	if (m_datasets->size() == 0)
	{
		statusMsg("No input given!");
		return;
	}
	statusMsg("");
	statusMsg("---------- SAMPLING STARTED ----------");
	statusMsg("Generating sampling parameter sets...");
	m_parameterSets = m_sampleGenerator->parameterSets(m_parameterRanges, m_parameters[spnNumberOfSamples].toInt());
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

	m_numDigits = requiredDigits(m_parameterSets->size());
	for (int i = 0; i < CONCURRENT_COMPUTATION_RUNS; ++i)
	{
		newSamplingRun();
	}
}

void iAImageSampler::computationFinished()
{
	iASampleOperation* op = dynamic_cast<iASampleOperation*>(QObject::sender());
	if (!op)
	{
		statusMsg("Invalid state: nullptr sender in computationFinished!");
		return;
	}
	int id = m_runningComputation[op];
	m_runningComputation.remove(op);
	iAPerformanceTimer::DurationType computationTime = op->duration();
	statusMsg(QString("Finished in %1 seconds. Output: %2\n")
		.arg(QString::number(computationTime))
		.arg(op->output()));
	m_computationDuration += computationTime;
	if (!op->success())
	{
		statusMsg("Computation was NOT successful.");
		if (!m_parameters[spnContinueOnError].toBool())
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
	iAParameterSet const & param = m_parameterSets->at(id);

	QString outputFolder(getOutputFolder(
		m_parameters[spnOutputFolder].toString(),
		m_parameters[spnSubfolderPerSample].toBool(), id, m_numDigits));
	QDir d(QDir::root());
	if (!QDir(outputFolder).exists() && !d.mkpath(outputFolder))
	{
		statusMsg(QString("Could not create output folder '%1'. Critical error, aborting sampling.").arg(outputFolder));
		m_aborted = true;
		if (m_runningComputation.size() == 0)
		{
			emit finished();
		}
		return;
	}
	QString outputFile(getOutputFileName(outputFolder, m_parameters[spnBaseName].toString(),
		m_parameters[spnSubfolderPerSample].toBool(), id, m_numDigits));
	QSharedPointer<iASingleResult> result = iASingleResult::create(id, *m_results.data(), param,
		outputFile);

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
	delete op;
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
