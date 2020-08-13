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
#include "iASampleFilter.h"

#include "dlg_samplingSettings.h"
#include "iAImageSampler.h"
#include "iAParameterGeneratorImpl.h"
#include "iAParameterNames.h"

#include <iAConsole.h>
#include <iAModalityList.h>
#include <iAModality.h>
#include <iAProgress.h>
#include <io/iAFileUtils.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QDir>


IAFILTER_CREATE(iASampleFilter)

iASampleFilter::iASampleFilter() :
	iAFilter("Sample Filter", "Image Ensembles",
		"Sample any internal filter or external algorithm.<br/>",
		1, 0),
	m_sampler(nullptr)
{
	addParameter(spnAlgorithmName, String, "");
	QStringList algorithmTypes;
	algorithmTypes << atBuiltIn << atExternal;
	addParameter(spnAlgorithmType, Categorical, algorithmTypes);
	addParameter(spnFilter, FilterName, "Image Quality");
	addParameter(spnExecutable, FileNameOpen, "");
	addParameter(spnParameterDescriptor, FileNameOpen, "");
	addParameter(spnAdditionalArguments, String, "");
	QStringList samplingMethods;
	auto& paramGens = getParameterGenerators();
	for (QSharedPointer<iAParameterGenerator> paramGen : paramGens)
	{
		samplingMethods << paramGen->name();
	}
	addParameter(spnSamplingMethod, Categorical, samplingMethods);
	addParameter(spnNumberOfSamples, Discrete, 100);
	addParameter(spnOutputFolder, Folder, "C:/sampling");
	addParameter(spnBaseName, String, "sample.mhd");
	addParameter(spnOverwriteOutput, Boolean, false);
	addParameter(spnSubfolderPerSample, Boolean, false);
	addParameter(spnComputeDerivedOutput, Boolean, false);
	addParameter(spnContinueOnError, Boolean, false);
	addParameter(spnCompressOutput, Boolean, true);
	addParameter(spnNumberOfLabels, Discrete, 2);
}

void iASampleFilter::performWork(QMap<QString, QVariant> const& parameters)
{
	// ITK_TYPED_CALL(sample, inputPixelType(), this, parameters);
	auto parameterSetGenerator = getParameterGenerator(parameters["Sampling method"].toString());
	if (!parameterSetGenerator)
	{
		return;
	}
	m_sampler = new iAImageSampler(
		m_input,
		parameters,
		m_parameterRanges,
		parameterSetGenerator,
		m_parameterRangeFile,
		m_parameterSetFile,
		m_derivedOutFile,
		m_samplingID,
		logger()
	);
	QObject::connect(m_sampler, &iAImageSampler::progress, progress(), &iAProgress::emitProgress);
	QObject::connect(m_sampler, &iAImageSampler::status, progress(), &iAProgress::setStatus);
	//connect(&sampler, &iAImageSampler::status, ...);
	QEventLoop loop;
	QObject::connect(m_sampler, &iAImageSampler::finished, &loop, &QEventLoop::quit);
	m_sampler->start();  //< returns as soon as first sampling run is started,
	loop.exec();	  //< so wait for finished event
}

void iASampleFilter::setParameters(QSharedPointer<iAModalityList> input, QSharedPointer<iAAttributes> parameterRanges,
	QString const& parameterRangeFile, QString const& parameterSetFile, QString const& derivedOutFile, int samplingID)
{
	// TODO: get parameter ranges and filenames in cmd/gui-agnostical way?
	m_input = input;
	m_parameterRanges = parameterRanges;

	m_parameterRangeFile = parameterRangeFile;
	m_parameterSetFile = parameterSetFile;
	m_derivedOutFile = derivedOutFile;
	m_samplingID = samplingID;
}

void iASampleFilter::abort()
{
	if (m_sampler)
	{
		m_sampler->abort();
	}
}

bool iASampleFilter::canAbort()
{
	return true;
}



IAFILTER_RUNNER_CREATE(iASampleFilterRunnerGUI);

bool iASampleFilterRunnerGUI::askForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant>& parameters,
	MdiChild* sourceMdi, MainWindow* mainWnd, bool /*askForAdditionalInput*/)
{
	iASampleFilter* sampleFilter = dynamic_cast<iASampleFilter*>(filter.data());
	if (!sampleFilter)
	{
		DEBUG_LOG("Invalid use of iASampleFilterRunnerGUI for a filter other than Sample Filter!");
		return false;
	}
	dlg_samplingSettings dlg(mainWnd, sourceMdi->modalities()->size(), parameters);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	dlg.getValues(parameters);
	QDir outputFolder(parameters[spnOutputFolder].toString());
	outputFolder.mkpath(".");
	if (parameters[spnComputeDerivedOutput].toBool() && parameters[spnNumberOfLabels].toInt() < 2)
	{
		DEBUG_LOG("'Number of labels' must not be smaller than 2!");
		return false;
	}
	auto parameterRanges = dlg.parameterRanges();
	QString outBaseName = QFileInfo(parameters[spnBaseName].toString()).completeBaseName();
	QString parameterRangeFile = outBaseName + "-parameterRanges.csv"; // iASEAFile::DefaultSMPFileName,
	QString parameterSetFile   = outBaseName + "-parameterSets.csv";   // iASEAFile::DefaultSPSFileName,
	QString derivedOutputFile  = outBaseName + "-derivedOutput.csv";   // iASEAFile::DefaultCHRFileName,

	int SamplingID = 0;
	sampleFilter->setParameters(sourceMdi->modalities(), parameterRanges, parameterRangeFile, parameterSetFile, derivedOutputFile, SamplingID);
	return true;
}