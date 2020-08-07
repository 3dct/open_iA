#pragma once
// Sample filter parameter names
const QString spnAlgorithmName("Algorithm name");
const QString spnAlgorithmType("Algorithm type");
const QString spnFilter("Filter");
const QString spnExecutable("Executable");
const QString spnParameterDescriptor("Parameter descriptor");
const QString spnAdditionalArguments("Additional arguments");
const QString spnSamplingMethod("Sampling method");
const QString spnNumberOfSamples("Number of samples");
const QString spnOutputFolder("Output folder");
const QString spnBaseName("Base name");
const QString spnSubfolderPerSample("Subfolder per sample");
const QString spnComputeDerivedOutput("Compute derived output");
const QString spnAbortOnError("Abort on errors");
const QString spnCompressOutput("Compress Output");
const QString spnNumberOfLabels("Number of labels");
// Valid values for algorithm type parameter:
const QString atBuiltIn("Built-in");
const QString atExternal("External");


// TODO: find better place (implementation currently in iAImageSampler.cpp):
QString getOutputFolder(QString const& baseFolder, bool createSubFolder, int sampleNr, int numDigits);
QString getOutputFileName(QString const& outputFolder, QString const& baseName,
	bool createSubFolder, int sampleNr, int numDigits);
int requiredDigits(int largestNumber);
