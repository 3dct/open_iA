// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageSampler.h"

#include "iAAttributes.h"
#include "iACommandRunner.h"
#include "iADerivedOutputCalculator.h"
#include "iASamplingMethod.h"
#include "iASingleResult.h"
#include "iASampleBuiltInFilterOperation.h"
#include "iAParameterNames.h"
#include "iASamplingResults.h"

#include <iAAttributeDescriptor.h>
#include <iADataSet.h>
#include <iAFileUtils.h>
#include <iALog.h>
#include <iAImageCoordinate.h>
#include <iANameMapper.h>
#include <iAProgress.h>
#include <iAStringHelper.h>

#include <QDir>
#include <QMap>
#include <QTextStream>

const int CONCURRENT_COMPUTATION_RUNS = 1;


iAPerformanceTimer m_computationTimer;

iAImageSampler::iAImageSampler(
		std::map<size_t, std::shared_ptr<iADataSet>> dataSets,
		QVariantMap const & parameters,
		std::shared_ptr<iAAttributes> parameterRanges,
		std::shared_ptr<iAAttributes> parameterSpecs,
		std::shared_ptr<iASamplingMethod> samplingMethod,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & derivedOutputFile,
		int samplingID,
		iALogger * logger,
		iAProgress * progress) :
	m_dataSets(dataSets),
	m_parameters(parameters),
	m_parameterRanges(parameterRanges),
	m_parameterSpecs(parameterSpecs),
	m_samplingMethod(samplingMethod),
	m_parameterRangeFile(parameterRangeFile),
	m_parameterSetFile  (parameterSetFile),
	m_derivedOutputFile (derivedOutputFile),
	m_curSample(0),
	m_aborted(false),
	m_computationDuration(0),
	m_derivedOutputDuration(0),
	m_samplingID(samplingID),
	m_logger(logger),
	m_progress(progress)
{
}

void iAImageSampler::statusMsg(QString const & msg)
{
	m_progress->setStatus(msg);
	LOG(lvlInfo, msg);
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
	QDir dir(QDir::root());
	if (!QDir(outputFolder).exists() && !dir.mkpath(outputFolder))
	{
		statusMsg(QString("Could not create output folder '%1'").arg(outputFolder));
		return;
	}
	QString outputFile(getOutputFileName(outputFolder, m_parameters[spnBaseName].toString(),
		m_parameters[spnSubfolderPerSample].toBool(), m_curSample, m_numDigits));
	iASampleOperation* op(nullptr);

	if (m_parameters[spnAlgorithmType].toString() == atBuiltIn)
	{
		QVariantMap singleRunParams;
		for (int i = 0; i < m_parameterCount; ++i)
		{
			auto desc = m_parameterRanges->at(i);
			singleRunParams.insert(desc->name(), paramSet.at(i));
		}
		op = new iASampleBuiltInFilterOperation(
			m_parameters[spnFilter].toString(),
			m_parameters[spnCompressOutput].toBool(),
			m_parameters[spnOverwriteOutput].toBool(),
			singleRunParams, m_dataSets, outputFile, m_logger);
	}
	else if (m_parameters[spnAlgorithmType].toString() == atExternal)
	{
		QStringList argumentList;
		argumentList << m_additionalArgumentList;
		argumentList << outputFile;

		for (auto ds: m_dataSets)
		{
			argumentList << ds.second->metaData(iADataSet::FileNameKey).toString();
		}

		for (int i = 0; i < m_parameterCount; ++i)
		{
			QString value;
			switch (m_parameterRanges->at(i)->valueType())
			{
			case iAValueType::Continuous:
				value = QString::number(paramSet.at(i).toDouble(), 'g', 12);
				break;
			case iAValueType::Discrete:
				value = QString::number(paramSet.at(i).toInt());
				break;
			case iAValueType::Categorical:
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
	++m_curSample;
	connect(op, &iASampleBuiltInFilterOperation::finished, this, &iAImageSampler::computationFinished);
	op->start();
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
	if (m_dataSets.size() == 0)
	{
		statusMsg("No input given!");
		return;
	}
	statusMsg("");
	statusMsg("---------- SAMPLING STARTED ----------");
	statusMsg("Generating sampling parameter sets...");
	m_parameterSets = m_samplingMethod->parameterSets(m_parameterRanges);
	if (!m_parameterSets)
	{
		statusMsg("No Parameters available!");
		return;
	}
	m_numDigits = requiredDigits(m_parameterSets->size());
	for (int paramSetIdx = 0; paramSetIdx < m_parameterSets->size(); ++paramSetIdx)
	{
		auto& paramSet = (*m_parameterSets)[paramSetIdx];
		for (int p = 0; p < m_parameterRanges->size(); ++p)
		{
			auto const & param = m_parameterRanges->at(p);
			if (param->valueType() == iAValueType::FileNameSave)
			{	// all output file names need to be adapted to output file name
				QString outputFolder(getOutputFolder(
					m_parameters[spnOutputFolder].toString(),
					m_parameters[spnSubfolderPerSample].toBool(), paramSetIdx, m_numDigits));
				QString outputFile(getOutputFileName(outputFolder, m_parameters[spnBaseName].toString(),
					m_parameters[spnSubfolderPerSample].toBool(), paramSetIdx, m_numDigits));
				auto value = pathFileBaseName(QFileInfo(outputFile)) + m_parameterSpecs->at(p)->defaultValue().toString();
				if (QFile::exists(value) && !m_parameters[spnOverwriteOutput].toBool())
				{
					LOG(lvlError, QString("Output file '%1' already exists! Aborting. "
						"Check 'Overwrite output' to overwrite existing files.").arg(value));
					return;
				}
				paramSet[p] = value;
			}
		}
	}
	LOG(lvlInfo, QString("Parameter combinations that will be sampled (%1):").arg(m_parameterSets->size()));
	for (auto parameterSet: *m_parameterSets.get())
	{
		LOG(lvlInfo, QString(joinQVariantAsString(parameterSet, ",")));
	}

	m_parameterCount = countAttributes(*m_parameterRanges.get(), iAAttributeDescriptor::Parameter);

	m_additionalArgumentList = splitPossiblyQuotedString(m_parameters[spnAdditionalArguments].toString());
	if (findAttribute(*m_parameterRanges.get(), "Performance") == -1)
	{
		std::shared_ptr<iAAttributeDescriptor> timeAttr(
			new iAAttributeDescriptor("Performance", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		m_parameterRanges->push_back(timeAttr);
		if (m_parameters[spnComputeDerivedOutput].toBool())
		{
			// add derived output to the attributes (which we want to set during sampling):
			std::shared_ptr<iAAttributeDescriptor> objectCountAttr(
				new iAAttributeDescriptor("Object Count", iAAttributeDescriptor::DerivedOutput, iAValueType::Discrete));
			std::shared_ptr<iAAttributeDescriptor> avgUncertaintyAttr(new iAAttributeDescriptor(
				"Average Uncertainty", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
			m_parameterRanges->push_back(objectCountAttr);
			m_parameterRanges->push_back(avgUncertaintyAttr);
		}
	}

	m_results = std::make_shared<iASamplingResults>(
		m_parameterRanges,
		m_samplingMethod->name(),
		m_parameters[spnOutputFolder].toString(),
		m_parameters[spnExecutable].toString(),
		m_parameters[spnAdditionalArguments].toString(),
		m_parameters[spnAlgorithmName].toString(),
		m_samplingID);

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
	statusMsg(QString("Finished in %1 seconds.%2\n")
		.arg(QString::number(computationTime))
		.arg(op->output().isEmpty() ? QString() : QString(" Output: %2").arg(op->output())) );
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
	std::shared_ptr<iASingleResult> result = iASingleResult::create(id, *m_results.get(), param,
		outputFile);

	result->setAttribute(m_parameterCount, computationTime);
	m_results->attributes()->at(m_parameterCount)->adjustMinMax(computationTime);

	if (m_parameters[spnComputeDerivedOutput].toBool())
	{
		// TODO: use external programs / built-in filters to calculate derived output
		iADerivedOutputCalculator * newCharCalc = new iADerivedOutputCalculator(result, m_parameterCount+1, m_parameterCount+2,
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
		m_progress->emitProgress(m_results->size() * 100.0 / m_parameterSets->size());
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

	std::shared_ptr<iASingleResult> result = m_runningDerivedOutput[charactCalc];
	m_results->attributes()->at(m_parameterCount+1)->adjustMinMax(result->attribute(m_parameterCount+1));
	m_results->attributes()->at(m_parameterCount+2)->adjustMinMax(result->attribute(m_parameterCount+2));

	// TODO: pass in from somewhere! Or don't store here at all? but what in case of a power outage/error?
	QString sampleMetaFile    = m_parameters[spnOutputFolder].toString() + "/" + m_parameterRangeFile;
	QString parameterSetFile  = m_parameters[spnOutputFolder].toString() + "/" + m_parameterSetFile;
	QString derivedOutputFile = m_parameters[spnOutputFolder].toString() + "/" + m_derivedOutputFile;
	m_results->addResult(result);
	m_progress->emitProgress(m_results->size() * 100.0 / m_parameterSets->size());
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

double iAImageSampler::estimatedTimeRemaining(double percent) const
{
	Q_UNUSED(percent);
	return
		(m_overallTimer.elapsed()/(m_curSample +1)) // average duration of one cycle
		* static_cast<double>(m_parameterSets->size()- m_curSample -1) // remaining cycles
	;
}

std::shared_ptr<iASamplingResults> iAImageSampler::results()
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
