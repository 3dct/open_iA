// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAParameterNames.h"

#include "iASamplingMethodImpl.h"  // for iAGlobalSensitivitySamplingMethod::Name

#include <QFileInfo>

#include <cmath>

const QString spnAlgorithmName("Algorithm name");
const QString spnAlgorithmType("Algorithm type");
const QString spnFilter("Filter");
const QString spnExecutable("Executable");
const QString spnParameterDescriptor("Parameter descriptor");
const QString spnAdditionalArguments("Additional arguments");
const QString spnSamplingMethod("Sampling method");
const QString spnNumberOfSamples("Number of samples");
const QString spnOutputFolder("Output directory");
const QString spnBaseName("Base name");
const QString spnOverwriteOutput("Overwrite output");
const QString spnSubfolderPerSample("Subfolder per sample");
const QString spnComputeDerivedOutput("Compute derived output");
const QString spnContinueOnError("Continue on error");
const QString spnCompressOutput("Compress output");
const QString spnNumberOfLabels("Number of labels");

// Parameters for general sensitivity sampling method:
const QString spnBaseSamplingMethod("Base sampling method");
const QString spnStarDelta("Sensitivity delta");
const QString spnStarStepNumber("Number of steps");

// Parameters for re-run sampling
const QString spnParameterSetFile("Parameter set file");

// Valid values for algorithm type parameter:
const QString atBuiltIn("Built-in");
const QString atExternal("External");

namespace iASamplingMethodName
{
	const QString Random("Random");
	const QString LatinHypercube("Latin HyperCube");
	const QString CartesianGrid("Cartesian Grid");
	const QString LocalSensitivity("Local Sensitivity");
	const QString GlobalSensitivity("Global sensitivity (star)");
	const QString GlobalSensitivitySmall("Global sensitivity (small star)");
	const QString RerunSampling("Re-run previous sampling");
}

const QString SampleFilterDescription(QString("Sample any internal filter or external algorithm.<br/>"

	"If '%1' sampling method is chosen, "
	"some additional options are available: <em>%2</em> determines "
	"the sampling method used to generate the base parameter set; then, "
	"for each of the parameter sets created by this method, "
	"additional samples are created. <em>%3</em> specifies the distance between "
	"samples as a ratio of the specified range for a given parameter (e.g. 0.05 would "
	"mean a step of 5% of the parameter range). 1 / <em>%4</em> number of samples will be created "
	"per parameter in a distance of x*<em>%3</em> "
	"(with x=1,2,3, ...) from the point in the parameter space determined by "
	"the parameter set. ")
	.arg(iASamplingMethodName::GlobalSensitivity)
	.arg(spnBaseSamplingMethod)
	.arg(spnStarDelta)
	.arg(spnStarStepNumber));

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
