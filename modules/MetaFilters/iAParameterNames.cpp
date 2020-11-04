/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
const QString spnSensitivityDelta("Sensitivity delta");
const QString spnSamplesPerPoint("Samples per point");

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
}

const QString SampleFilterDescription(QString("Sample any internal filter or external algorithm.<br/>"

	"If '%1' sampling method is chosen, "
	"some additional options are available: <em>%2</em> determines "
	"the sampling method used to generate the base parameter set; then, "
	"for each of the parameter sets created by this method, <em>%3</em> "
	"additional samples are created. <em>%4</em> specifies the distance between "
	"samples as a ratio of the specified range for a given parameter (e.g. 0.05 would "
	"mean a step of 5% of the parameter range). <em>%3</em> samples will created "
	"per parameter in a distance of x*<em>%4</em> "
	"(with x=1,2,3, ...) from the point in the parameter space determined by "
	"the parameter set. If <em>%3</em> is left at the default value of 0, "
	"the number of additional samples is "
	"automatically determined as the number of samples fitting into the "
	"parameter's whole range, i.e. 1 / <em>%4</em> (this will also be used as upper limit).")
	.arg(iASamplingMethodName::GlobalSensitivity)
	.arg(spnBaseSamplingMethod)
	.arg(spnSamplesPerPoint)
	.arg(spnSensitivityDelta));

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
