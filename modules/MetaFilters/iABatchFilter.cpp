/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iABatchFilter.h"

#include "iAParameterNames.h"

#include <iAFilterRegistry.h>
#include <iAProgress.h>

// io
#include <iAFileTypeRegistry.h>

// base
#include <iAAttributeDescriptor.h>
#include <iAConnector.h>
#include <iALog.h>
#include <iAITKIO.h>
#include <iAFileUtils.h>
#include <iAStringHelper.h>
#include <qthelper/iAQtEndl.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

iABatchFilter::iABatchFilter():
	iAFilter("Batch...", "",
		QString(
		"Runs a filter on a selected set of images.<br/>"
		"Specify an <em>Image folder</em> which contains the images to be processed. "
		"<em>Recursive</em> toggles whether or not to also consider subdirectories. "
		"The <em>File mask</em> is applied to match which files in the given folder are processed "
		"(separate multiple masks via ';', e.g. '*.mhd;*.tif'. "
		"The specified <em>Filter</em> is applied to all files specified with above settings, "
		"every time executed with the same set of <em>Parameters</em>.<br/>"
		"Specify file names under <em>Additional input</em> to pass additional images "
		"(e.g. a reference image for Segmentation Metrics) to the filter; if the file name "
		"contains spaces, put it between double quotes.<br/>"
		"For image output, set <em>Output directory</em> for a folder where to write all "
		"output images. If this is not set, output files will be written to the same folder "
		"as the input. The output folder can be a subfolder of the input folder, the list of "
		"files to be processed is read at the beginning, and thus output files will not again be "
		"batch processed. The <em>Output suffix</em> will be appended before the file extension. "
		"If <em>Overwrite output</em> is enabled, the result filename will be used even if a "
		"file with that name already exists. Otherwise, a new filename will be created by adding "
		"an increasing number to the suffix until there is no such file yet. So, to overwrite "
		"the input file with the output file, leave both 'Output directory' and 'Output suffix' "
		"empty, and enable 'Overwrite output'. If more than 1 output is available, an additional "
		"index (0 to output count-1) will be added to all outputs.<br/>"
		"When <em>Output csv file</em> is not empty, all output values produced by the filter "
		"will be written to the file name given here, one row per image and filter. "
		"If the output csv file exists, and <em>Append to output</em> is enabled, "
		"the output values are appended at the end of each line. Note that this tool does not "
		"try to find the matching filename in the given output csv, and hence the appending only "
		"works properly (i.e. appends values to the proper file) if no images have been added or "
		"removed from the folder, and also the recursive and file mask options are the same as "
		"with the batch run that created the file in the first place. "
		"If <em>Add filename</em> is enabled, then the name of the file processed for that "
		"line will be appended before the first output value from that file."
		"When <em>%1</em> is enabled, then batch processing will continue with the next file "
		"in case there is an error. If it is disabled, an error will interrupt the whole batch run. "
		"Under <em>Work on</em> it can be specified whether the batched filter should get passed "
		"only files, only folders, or both files and folders."
		"<em>Output format</em> specifies the file format for the output image(s)."
		).arg(spnContinueOnError), 0, 0, true)
{
	QStringList filesFoldersBoth;
	filesFoldersBoth << "Files" << "Folders" << "Both Files and Folders";
	addParameter("Image folder", iAValueType::Folder, "");
	addParameter("Recursive", iAValueType::Boolean, false);
	addParameter("File mask", iAValueType::String, "*.mhd");
	addParameter(spnFilter, iAValueType::FilterName, "Image Quality");
	addParameter("Parameters", iAValueType::FilterParameters, "");
	addParameter("Additional Input", iAValueType::FileNamesOpen, "");
	addParameter(spnOutputFolder, iAValueType::Folder, "");
	addParameter("Output suffix", iAValueType::String, "");
	addParameter(spnOverwriteOutput, iAValueType::Boolean, false);
	addParameter(spnCompressOutput, iAValueType::Boolean, true);
	addParameter("Output csv file", iAValueType::FileNameSave, ".csv");
	addParameter("Append to output", iAValueType::Boolean, true);
	addParameter("Add filename", iAValueType::Boolean, true);
	addParameter(spnContinueOnError, iAValueType::Boolean, false);
	addParameter("Work on", iAValueType::Categorical, filesFoldersBoth);
	QStringList outputFormat;
	outputFormat << "Same as input"
		<< "MetaImage (*.mhd)";
	addParameter("Output format", iAValueType::Categorical, outputFormat);
}

void iABatchFilter::performWork(QVariantMap const & parameters)
{
	auto filter = iAFilterRegistry::filter(parameters[spnFilter].toString());
	if (!filter)
	{
		addMsg(QString("Batch: Cannot run filter '%1', it does not exist!").arg(parameters[spnFilter].toString()));
		return;
	}
	QVariantMap filterParams;
	QStringList filterParamStrs = splitPossiblyQuotedString(parameters["Parameters"].toString());
	if (filter->parameters().size() != filterParamStrs.size())
	{
		addMsg(QString("Batch: Invalid number of parameters: %1 expected, %2 given!")
			.arg(filter->parameters().size())
			.arg(filterParamStrs.size()));
		return;
	}
	QString batchDir = parameters["Image folder"].toString();
	std::vector<std::shared_ptr<iADataSet>> inputImages;
	QStringList additionalInput = splitPossiblyQuotedString(parameters["Additional Input"].toString());
	for (QString fileName : additionalInput)
	{
		if (isAborted())
		{
			break;
		}
		fileName = MakeAbsolute(batchDir, fileName);
		auto io = iAFileTypeRegistry::createIO(fileName);
		QVariantMap dummyParams;    // TODO: CHECK whether I/O requires other parameters and error in that case!
		iAProgress dummyProgress;
		auto dataSets = io->load(fileName, dummyParams, &dummyProgress);
		for (auto d : dataSets)
		{
			inputImages.push_back(d);
		}
	}

	for (int i = 0; i < filterParamStrs.size(); ++i)
	{
		filterParams.insert(filter->parameters()[i]->name(), filterParamStrs[i]);
	}

	QString outputFile = parameters["Output csv file"].toString();
	QStringList outputBuffer;
	if (parameters["Append to output"].toBool() && QFile(outputFile).exists())
	{
		QFile file(outputFile);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
			while (!textStream.atEnd())
			{
				outputBuffer << textStream.readLine();
			}
			file.close();
		}
	}
	filter->setLogger(logger());

	QStringList filters = parameters["File mask"].toString().split(";");

	if (!QFileInfo(batchDir).exists() || !QFileInfo(batchDir).isDir())
	{
		addMsg("Path given as 'Image folder' either does not exist or is not a folder!");
		return;
	}
	QStringList files;
	QFlags<FilesFolders> filesFolders;
	if (parameters["Work on"].toString().contains("Files"))
	{
		filesFolders |= Files;
	}
	else if (parameters["Work on"].toString().contains("Folders"))
	{
		filesFolders |= Folders;
	}
	FindFiles(batchDir, filters, parameters["Recursive"].toBool(), files, filesFolders);
	QString outDir(parameters[spnOutputFolder].toString());
	if (!outDir.isEmpty())
	{
		QFileInfo fi(outDir);
		if (fi.exists() && !fi.isDir())
		{
			addMsg(QString("Path given as 'Output directory' (%1) does not denote a folder!").arg(outDir));
			return;
		}
		else if (!fi.exists())
		{
			if (!QDir(fi.absoluteFilePath()).mkpath("."))
			{
				addMsg(QString("Could not create output directory '%1'").arg(outDir));
				return;
			}
		}
	}
	else
	{
		outDir = batchDir;
	}
	QString outSuffix = parameters["Output suffix"].toString();
	bool overwrite = parameters[spnOverwriteOutput].toBool();
	bool useCompression = parameters[spnCompressOutput].toBool();
	int curLine = 0;
	for (QString fileName : files)
	{
		progress()->setStatus(QString("Processing file %1...").arg(fileName));
		try
		{
			filter->clearInput();
			if (QFileInfo(fileName).isDir())
			{
				filterParams["Folder name"] = fileName;
			}
			else
			{
				if (filter->requiredInputs() > 0)
				{
					auto io = iAFileTypeRegistry::createIO(fileName);
					QVariantMap dummyParams;    // TODO: CHECK whether I/O requires other parameters and error in that case!
					iAProgress dummyProgress;
					auto dataSets = io->load(fileName, dummyParams, &dummyProgress);
					for (auto d: dataSets)
					{
						filter->addInput(d);
					}
					for (int i = 0; i < inputImages.size(); ++i)
					{
						filter->addInput(inputImages[i]);
					}
				}
				/*
				// attempt to make input filename accessible to filter,
				// in order for filters to be able to deduce their output names automatically
				// -> now rather FileNameSave parameters get set by Batch and Sample filters!
				for (int i = 0; i < inputImages.size(); ++i)
				{
					filterParams[QString("Input file %1").arg(i)] = fileName;
				}
				*/
			}
			for (auto const& param: filter->parameters())
			{
				if (param->valueType() == iAValueType::FileNameSave)
				{	// all output file names need to be adapted to output file name;
					// merge with code in iASampleBuiltInFilterOperation?
					auto value = pathFileBaseName(QFileInfo(fileName)) + param->defaultValue().toString();
					if (QFile::exists(value) && !overwrite)
					{
						LOG(lvlError, QString("Output file '%1' already exists! Aborting. "
							"Check '%2' to overwrite existing files.").arg(value).arg(spnOverwriteOutput));
						return;
					}
					filterParams[param->name()] = value;
				}
			}
			filter->run(filterParams);
			if (curLine == 0)
			{
				QStringList captions;
				if (parameters["Add filename"].toBool())
				{
					captions << "filename";
				}
				for (auto outValue : filter->outputValues())
				{
					QString curCap(outValue.first);
					curCap.replace(",", "");
					captions << curCap;
				}
				if (outputBuffer.empty())
				{
					outputBuffer.append("");
				}
				outputBuffer[0] += (outputBuffer[0].isEmpty() || captions.empty() ? "" : ",") + captions.join(",");
				++curLine;
			}
			if (curLine >= outputBuffer.size())
			{
				outputBuffer.append("");
			}
			QStringList values;
			QString relFileName = MakeRelative(batchDir, fileName);
			if (parameters["Add filename"].toBool())
			{
				values << relFileName;
			}
			for (auto outValue : filter->outputValues())
			{
				values.append(outValue.second.toString());
			}
			QString textToAdd = (outputBuffer[curLine].isEmpty() || values.empty() ? "" : ",") + values.join(",");
			outputBuffer[curLine] += textToAdd;
			++curLine;
			for (size_t o = 0; o < filter->finalOutputCount(); ++o)
			{
				QFileInfo fi(outDir + "/" + relFileName);
				QString multiFileSuffix = filter->finalOutputCount() > 1 ? QString::number(o) : "";
				QString outName = QString("%1/%2%3%4.%5").arg(fi.absolutePath()).arg(
					parameters["Output format"].toString().contains("MetaImage") ? fi.fileName() : fi.baseName())
					.arg(outSuffix).arg(multiFileSuffix).arg(
						parameters["Output format"].toString().contains("MetaImage")? "mhd" :	fi.completeSuffix());
				int overwriteSuffix = 0;
				while (!overwrite && QFile(outName).exists())
				{
					outName = QString("%1/%2%3%4-%5.%6").arg(fi.absolutePath()).arg(
						parameters["Output format"].toString().contains("MetaImage") ? fi.fileName() : fi.baseName())
						.arg(outSuffix).arg(multiFileSuffix).arg(overwriteSuffix).arg(
							parameters["Output format"].toString().contains("MetaImage") ? "mhd" :	fi.completeSuffix());
					++overwriteSuffix;
				}
				if (!QDir(fi.absolutePath()).exists() && !QDir(fi.absolutePath()).mkpath("."))
				{
					addMsg(QString("Error creating output directory %1, skipping writing output file %2")
						.arg(fi.absolutePath()).arg(outName));
				}
				else
				{
					auto io = iAFileTypeRegistry::createIO(fileName);
					QVariantMap writeParamValues;    // TODO: CHECK whether I/O requires other parameters and error in that case!
					iAProgress dummyProgress;
					writeParamValues[iAFileIO::CompressionStr] = useCompression;
					io->save(outName, { filter->output(o) }, writeParamValues, &dummyProgress);
				}
			}
		}
		catch (std::exception & e)
		{
			LOG(lvlError, QString("Batch processing: Error while processing file '%1': %2").arg(fileName).arg(e.what()));
			if (!parameters["Continue on error"].toBool())
			{
				throw;
			}
		}
		progress()->emitProgress( (curLine - 1.0) * 100.0 / files.size() );
		if (isAborted())
		{
			break;
		}
	}

	if (!isAborted() && !outputFile.isEmpty())
	{
		QFile file(outputFile);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
			for (QString line : outputBuffer)
			{
				textStream << line << QTENDL;
			}
			file.close();
		}
	}
}
