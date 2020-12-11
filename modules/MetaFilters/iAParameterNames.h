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
#pragma once

#include "MetaFilters_export.h"

#include <QString>

// Sample/Batch filter parameter names
MetaFilters_API extern const QString spnAlgorithmName;
MetaFilters_API extern const QString spnAlgorithmType;
MetaFilters_API extern const QString spnFilter;
MetaFilters_API extern const QString spnExecutable;
MetaFilters_API extern const QString spnParameterDescriptor;
MetaFilters_API extern const QString spnAdditionalArguments;
MetaFilters_API extern const QString spnSamplingMethod;
MetaFilters_API extern const QString spnNumberOfSamples;
MetaFilters_API extern const QString spnOutputFolder;
MetaFilters_API extern const QString spnBaseName;
MetaFilters_API extern const QString spnOverwriteOutput;
MetaFilters_API extern const QString spnSubfolderPerSample;
MetaFilters_API extern const QString spnComputeDerivedOutput;
MetaFilters_API extern const QString spnContinueOnError;
MetaFilters_API extern const QString spnCompressOutput;
MetaFilters_API extern const QString spnNumberOfLabels;

// Parameters for general sensitivity sampling method:
MetaFilters_API extern const QString spnBaseSamplingMethod;
MetaFilters_API extern const QString spnSensitivityDelta;
MetaFilters_API extern const QString spnSamplesPerPoint;

// Valid values for algorithm type parameter:
MetaFilters_API extern const QString atBuiltIn;
MetaFilters_API extern const QString atExternal;

MetaFilters_API extern const QString SampleFilterDescription;

namespace iASamplingMethodName
{
	MetaFilters_API extern const QString Random;
	MetaFilters_API extern const QString LatinHypercube;
	MetaFilters_API extern const QString CartesianGrid;
	MetaFilters_API extern const QString LocalSensitivity;
	MetaFilters_API extern const QString GlobalSensitivity;
}

QString getOutputFolder(QString const& baseFolder, bool createSubFolder, int sampleNr, int numDigits);
QString getOutputFileName(QString const& outputFolder, QString const& baseName,
	bool createSubFolder, int sampleNr, int numDigits);
