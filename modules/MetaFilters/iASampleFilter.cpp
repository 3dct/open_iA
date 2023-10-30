// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASampleFilter.h"

#include "iAImageSampler.h"
#include "iASamplingMethodImpl.h"
#include "iAParameterNames.h"

#include <iADataSet.h>

#include <QEventLoop>


iASampleFilter::iASampleFilter() :
	iAFilter("Sample Filter", "Image Ensembles",
		SampleFilterDescription,
		1, 0, true),
	m_sampler(nullptr)
{
	addParameter(spnAlgorithmName, iAValueType::String, "");
	QStringList algorithmTypes;
	algorithmTypes << atBuiltIn << atExternal;
	addParameter(spnAlgorithmType, iAValueType::Categorical, algorithmTypes);
	addParameter(spnFilter, iAValueType::FilterName, "Image Quality");
	addParameter(spnExecutable, iAValueType::FileNameOpen, "");
	addParameter(spnParameterDescriptor, iAValueType::FileNameOpen, "");
	addParameter(spnAdditionalArguments, iAValueType::String, "");
	QStringList samplingMethods(samplingMethodNames());
	addParameter(spnSamplingMethod, iAValueType::Categorical, samplingMethods);
	addParameter(spnNumberOfSamples, iAValueType::Discrete, 100);
	addParameter(spnOutputFolder, iAValueType::Folder, "C:/sampling");
	addParameter(spnBaseName, iAValueType::String, "sample.mhd");
	addParameter(spnOverwriteOutput, iAValueType::Boolean, false);
	addParameter(spnSubfolderPerSample, iAValueType::Boolean, false);
	addParameter(spnComputeDerivedOutput, iAValueType::Boolean, false);
	addParameter(spnContinueOnError, iAValueType::Boolean, false);
	addParameter(spnCompressOutput, iAValueType::Boolean, true);
	addParameter(spnNumberOfLabels, iAValueType::Discrete, 2);
	
	samplingMethods.removeAll(iASamplingMethodName::GlobalSensitivity);
	// parameters only required for "Global sensitivity (star)" sampling:
	addParameter(spnBaseSamplingMethod, iAValueType::Categorical, samplingMethods);
	addParameter(spnStarDelta, iAValueType::Continuous, 0.1);
	// parameter only required for "Global sensitivity (star small)" sampling:
	addParameter(spnStarStepNumber, iAValueType::Continuous, 0.1);
}

void iASampleFilter::performWork(QVariantMap const& parameters)
{
	// ITK_TYPED_CALL(sample, inputScalarType(), this, parameters);
	auto samplingMethod = createSamplingMethod(parameters);
	if (!samplingMethod)
	{
		return;
	}
	if (samplingMethod->supportsSamplesPerParameter())
	{
		samplingMethod->setSamplesPerParameter(m_numOfSamplesPerParameter);
	}
	else
	{
		samplingMethod->setSampleCount(parameters[spnNumberOfSamples].toInt(), m_parameterRanges);
	}
	m_sampler = new iAImageSampler(
		m_input,
		parameters,
		m_parameterRanges,
		m_parameterSpecs,
		samplingMethod,
		m_parameterRangeFile,
		m_parameterSetFile,
		m_derivedOutFile,
		m_samplingID,
		logger(),
		progress()
	);
	QEventLoop loop;
	QObject::connect(m_sampler, &iAImageSampler::finished, &loop, &QEventLoop::quit);
	m_sampler->start();  //< returns as soon as first sampling run is started,
	loop.exec();	     //< so wait for finished event
}

bool iASampleFilter::checkParameters(QVariantMap const& paramValues)
{
	for (auto const & param : parameters())
	{
		if ( (param->name() == spnFilter && paramValues[spnAlgorithmType].toString() == atExternal) ||
			((param->name() == spnExecutable || param->name() == spnParameterDescriptor) &&
				paramValues[spnAlgorithmType].toString() == atBuiltIn) )
		{
			continue;
		}
		if (!defaultParameterCheck(param, paramValues[param->name()]))
		{
			return false;
		}
	}
	return true;
}

void iASampleFilter::setParameters(std::map<size_t, std::shared_ptr<iADataSet>> input, std::shared_ptr<iAAttributes> parameterRanges,
	std::shared_ptr<iAAttributes> parameterSpecs,
	QString const& parameterRangeFile, QString const& parameterSetFile, QString const& derivedOutFile, int samplingID,
	std::vector<int> numOfSamplesPerParameter)
{
	// TODO: get parameter ranges and filenames in cmd/gui-agnostical way?
	m_input = input;
	m_parameterRanges = parameterRanges;
	m_parameterSpecs = parameterSpecs;
	m_parameterRangeFile = parameterRangeFile;
	m_parameterSetFile = parameterSetFile;
	m_derivedOutFile = derivedOutFile;
	m_samplingID = samplingID;
	m_numOfSamplesPerParameter = numOfSamplesPerParameter;
}

void iASampleFilter::abort()
{
	if (m_sampler)
	{
		m_sampler->abort();
	}
}


