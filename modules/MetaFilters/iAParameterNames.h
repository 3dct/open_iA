// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

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
MetaFilters_API extern const QString spnStarDelta;
MetaFilters_API extern const QString spnStarStepNumber;

// Parameters for re-run sampling
MetaFilters_API extern const QString spnParameterSetFile;

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
	MetaFilters_API extern const QString GlobalSensitivitySmall;
	MetaFilters_API extern const QString RerunSampling;
}

QString getOutputFolder(QString const& baseFolder, bool createSubFolder, int sampleNr, int numDigits);
QString getOutputFileName(QString const& outputFolder, QString const& baseName,
	bool createSubFolder, int sampleNr, int numDigits);
