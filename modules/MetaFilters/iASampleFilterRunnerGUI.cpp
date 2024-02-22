// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASampleFilterRunnerGUI.h"

#include "iASamplingSettingsDlg.h"

#include <iAParameterNames.h>
#include <iASampleFilter.h>

#include <iALog.h>

#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <QDir>
#include <QFileInfo>

IAFILTER_RUNNER_CREATE(iASampleFilterRunnerGUI);

bool iASampleFilterRunnerGUI::askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap& parameters,
	iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool /*askForAdditionalInput*/)
{
	iASampleFilter* sampleFilter = dynamic_cast<iASampleFilter*>(filter.get());
	if (!sampleFilter)
	{
		LOG(lvlError, "Invalid use of iASampleFilterRunnerGUI for a filter other than Sample Filter!");
		return false;
	}
	iASamplingSettingsDlg dlg(mainWnd, static_cast<int>(sourceMdi->dataSetMap().size()), parameters);
	if (dlg.exec() != QDialog::Accepted)
	{
		LOG(lvlInfo, "Aborted sampling.");
		return false;
	}

	dlg.getValues(parameters);
	QDir outputFolder(parameters[spnOutputFolder].toString());
	outputFolder.mkpath(".");
	if (parameters[spnComputeDerivedOutput].toBool() && parameters[spnNumberOfLabels].toInt() < 2)
	{
		LOG(lvlError, "'Number of labels' must not be smaller than 2!");
		return false;
	}
	QString outBaseName = QFileInfo(parameters[spnBaseName].toString()).completeBaseName();
	QString parameterRangeFile = outBaseName + "-parameterRanges.csv"; // iASEAFile::DefaultSMPFileName,
	QString parameterSetFile   = outBaseName + "-parameterSets.csv";   // iASEAFile::DefaultSPSFileName,
	QString derivedOutputFile  = outBaseName + "-derivedOutput.csv";   // iASEAFile::DefaultCHRFileName,

	int SamplingID = 0;
	sampleFilter->setParameters(sourceMdi->dataSetMap(), dlg.parameterRanges(), dlg.parameterSpecs(),
		parameterRangeFile, parameterSetFile, derivedOutputFile, SamplingID, dlg.numOfSamplesPerParameter());
	return true;
}
