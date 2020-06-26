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

#include <iAConsole.h>
#include <iAModalityList.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QDir>


class iASampleParameters
{
public:
	QSharedPointer<iAModalityList const> modalities;
	QSharedPointer<iAAttributes> parameterRanges;
	QSharedPointer<iAParameterGenerator> sampleGenerator;
	int sampleCount;
	int labelCount;
	QString outputFolder,
		parameterRangeFile,
		parameterSetFile,
		derivedOutputFile,
		executable,
		additionalArguments,
		algorithmName,
		outBaseName;
	bool useSeparateOutputFolder, computeDerivedOutput;
	int samplingID;
};

IAFILTER_CREATE(iASampleFilter)

iASampleFilter::iASampleFilter() :
	iAFilter("Sample Filter", "Image Ensembles",
		"Sample any internal filter or external algorithm<br/>", 1, 0)
{
	addParameter("Algorithm Name", String, "");
	QStringList algorithmTypes;
	algorithmTypes << "BuiltIn" << "External";
	addParameter("Algorithm Type", Categorical, algorithmTypes);
	addParameter("Filter", FilterName, "Image Quality");
	addParameter("Executable", FileNameOpen, "");
	addParameter("Parameter Descriptor", FileNameOpen, "");
	addParameter("Additional Arguments", String, "");
	QStringList samplingMethods;
	auto& paramGens = GetParameterGenerators();
	for (QSharedPointer<iAParameterGenerator> paramGen : paramGens)
	{
		samplingMethods << paramGen->name();
	}
	addParameter("Sampling Method", Categorical, samplingMethods);
	addParameter("Number of samples", Discrete, 100);
	addParameter("Output folder", Folder, "C:/sampling");
	addParameter("Base name", FileNameSave, "sample.mhd");
	addParameter("Subfolder per sample", Boolean, false);
	addParameter("Compute derived output", Boolean, false);
	addParameter("Number of labels", Discrete, 2);
}

void iASampleFilter::performWork(QMap<QString, QVariant> const& parameters)
{
	// ITK_TYPED_CALL(sample, inputPixelType(), this, parameters);
	iAImageSampler sampler(
		m_p->modalities,
		m_p->parameterRanges,
		m_p->sampleGenerator,
		m_p->sampleCount,
		m_p->labelCount,
		m_p->outputFolder,
		m_p->parameterRangeFile,
		m_p->parameterSetFile,
		m_p->derivedOutputFile,
		m_p->executable,
		m_p->additionalArguments,
		m_p->algorithmName,
		m_p->outBaseName,
		m_p->useSeparateOutputFolder,
		m_p->computeDerivedOutput,
		m_p->samplingID
	);
	/*
	m_dlgProgress = new dlg_progress(this, m_sampler, m_sampler, "Sampling Progress");
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	mdiChild->tabifyDockWidget(this, m_dlgProgress);
	connect(m_sampler.data(), &iAImageSampler::finished, this, &dlg_GEMSeControl::samplingFinished);
	connect(m_sampler.data(), &iAImageSampler::Progress, m_dlgProgress, &dlg_progress::setProgress);
	connect(m_sampler.data(), &iAImageSampler::Status, m_dlgProgress, &dlg_progress::setStatus);

	*/
	// trigger parameter set creation & sampling (in foreground with progress bar for now)
	sampler.run();
}

void iASampleFilter::setParameters(QSharedPointer<iASampleParameters> p)
{
	m_p = p;
}






IAFILTER_RUNNER_CREATE(iASampleFilterRunner);

bool iASampleFilterRunner::askForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant>& parameters,
	MdiChild* sourceMdi, MainWindow* mainWnd, bool /*askForAdditionalInput*/)
{
	iASampleFilter* sampleFilter = dynamic_cast<iASampleFilter*>(filter.data());
	if (!sampleFilter)
	{
		DEBUG_LOG("Invalid use of iASampleFilterRunner for filter other than Sample Filter!");
		return false;
	}
	dlg_samplingSettings dlg(mainWnd, sourceMdi->modalities()->size(), parameters);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	dlg.getValues(parameters);
	// get parameter ranges
	QSharedPointer<iASampleParameters> p(new iASampleParameters());
	p->outputFolder = dlg.outputFolder();
	QDir outputFolder(p->outputFolder);
	outputFolder.mkpath(".");
	if (dlg.computeDerivedOutput() && dlg.labelCount() < 2)
	{
		DEBUG_LOG("Label Count must not be smaller than 2!");
		return false;
	}
	p->labelCount = dlg.labelCount();
	p->parameterRanges = dlg.parameterRanges();
	p->sampleGenerator = dlg.generator();
	p->sampleCount = dlg.sampleCount();
	p->executable =	dlg.executable();
	p->additionalArguments = dlg.additionalArguments();
	p->algorithmName = dlg.algorithmName();
	p->outBaseName = dlg.outBaseName();
	p->useSeparateOutputFolder = dlg.useSeparateFolder();
	p->computeDerivedOutput = dlg.computeDerivedOutput();
	// TOOD: 
	// iASEAFile::DefaultSMPFileName,
	// iASEAFile::DefaultSPSFileName,
	// iASEAFile::DefaultCHRFileName,
	p->parameterRangeFile = dlg.outBaseName() + "-parameterRanges.csv";
	p->parameterSetFile = dlg.outBaseName() + "-parameterSets.csv";
	p->derivedOutputFile = dlg.outBaseName() + "-derivedOutput.csv";
	// TODO: check if ID is needed ?
	p->samplingID = 0; 
	sampleFilter->setParameters(p);
	return true;
}