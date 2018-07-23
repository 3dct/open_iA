/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilterRegistry.h"
#include "iAProgress.h"
#include "iAStringHelper.h"
#include "io/iAITKIO.h"
#include "io/iAFileUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

iABatchFilter::iABatchFilter():
	iAFilter("Batch...", "",
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
		"When <em>Continue on error</em> is enabled, then batch processing will continue with the next file "
		"in case there is an error. If it is disabled, an error will interrupt the whole batch run. "
		"Under <em>Work on</em> it can be specified whether the batched filter should get passed "
		"only files, only folders, or both files and folders."
		"<em>Output format</em> specifies the file format for the output image(s).", 0, 0)
{
	QStringList filesFoldersBoth;
	filesFoldersBoth << "Files" << "Folders" << "Both Files and Folders";
	AddParameter("Image folder", Folder, "");
	AddParameter("Recursive", Boolean, false);
	AddParameter("File mask", String, "*.mhd");
	AddParameter("Filter", FilterName, "Image Quality");
	AddParameter("Parameters", FilterParameters, "");
	AddParameter("Additional Input", FileNamesOpen, "");
	AddParameter("Output directory", Folder, "");
	AddParameter("Output suffix", String, "");
	AddParameter("Overwrite output", Boolean, false);
	AddParameter("Compress output", Boolean, true);
	AddParameter("Output csv file", FileNameSave, "");
	AddParameter("Append to output", Boolean, true);
	AddParameter("Add filename", Boolean, true);
	AddParameter("Continue on error", Boolean, true);
	AddParameter("Work on", Categorical, filesFoldersBoth);
	QStringList outputFormat;
	outputFormat << "Same as input"
		<< "MetaImage (*.mhd)";
	AddParameter("Output format", Categorical, outputFormat);
}

void iABatchFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	auto filter = iAFilterRegistry::Filter(parameters["Filter"].toString());
	if (!filter)
	{
		AddMsg(QString("Batch: Cannot run filter '%1', it does not exist!").arg(parameters["Filter"].toString()));
		return;
	}
	QMap<QString, QVariant> filterParams;
	QStringList filterParamStrs = SplitPossiblyQuotedString(parameters["Parameters"].toString());
	if (filter->Parameters().size() != filterParamStrs.size())
	{
		AddMsg(QString("Batch: Invalid number of parameters: %1 expected, %2 given!")
			.arg(filter->Parameters().size())
			.arg(filterParamStrs.size()));
		return;
	}
	QString batchDir = parameters["Image folder"].toString();
	iAConnector* con = new iAConnector();
	QVector<iAConnector*> inputImages;
	QStringList additionalInput = SplitPossiblyQuotedString(parameters["Additional Input"].toString());
	for (QString fileName : additionalInput)
	{
		fileName = MakeAbsolute(batchDir, fileName);
		auto newCon = new iAConnector();
		iAITKIO::ScalarPixelType pixelType;
		iAITKIO::ImagePointer img = iAITKIO::readFile(fileName, pixelType, false);
		newCon->SetImage(img);
		inputImages.push_back(newCon);
	}

	for (int i=0; i<filterParamStrs.size(); ++i)
		filterParams.insert(filter->Parameters()[i]->Name(), filterParamStrs[i]);

	QString outputFile = parameters["Output csv file"].toString();
	QStringList outputBuffer;
	if (parameters["Append to output"].toBool() && QFile(outputFile).exists())
	{
		QFile file(outputFile);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
			while (!textStream.atEnd())
				outputBuffer << textStream.readLine();
			file.close();
		}
	}
	iAProgress p;	// dummy progress swallowing progress from filter which we don't want to propagate
	filter->SetProgress(&p);
	filter->SetLogger(Logger());

	QStringList filters = parameters["File mask"].toString().split(";");
	
	if (!QFileInfo(batchDir).exists() || !QFileInfo(batchDir).isDir())
	{
		AddMsg("Path given as 'Image folder' either does not exist or is not a folder!");
		return;
	}
	QStringList files;
	QFlags<FilesFolders> filesFolders;
	if (parameters["Work on"].toString().contains("Files"))
		filesFolders |= Files;
	else if (parameters["Work on"].toString().contains("Folders"))
		filesFolders |= Folders;
	FindFiles(batchDir, filters, parameters["Recursive"].toBool(), files, filesFolders);
	QString outDir(parameters["Output directory"].toString());
	if (!outDir.isEmpty())
	{
		QFileInfo fi(outDir);
		if (fi.exists() && !fi.isDir())
		{
			AddMsg(QString("Path given as 'Output directory' (%1) does not denote a folder!").arg(outDir));
			return;
		}
		else if (!fi.exists())
		{
			if (!QDir(fi.absoluteFilePath()).mkpath("."))
			{
				AddMsg(QString("Could not create output directory '%1'").arg(outDir));
				return;
			}
		}
	}
	else
		outDir = batchDir;
	QString outSuffix = parameters["Output suffix"].toString();
	bool overwrite = parameters["Overwrite output"].toBool();
	bool useCompression = parameters["Compress output"].toBool();
	size_t curLine = 0;
	for (QString fileName : files)
	{
		try
		{
			filter->ClearInput();
			iAConnector con;
			if (QFileInfo(fileName).isDir())
			{
				filterParams["Folder name"] = fileName;
			}
			else
			{
				if (filter->RequiredInputs() > 0)
				{
					iAITKIO::ScalarPixelType pixelType;
					iAITKIO::ImagePointer img = iAITKIO::readFile(fileName, pixelType, false);
					con.SetImage(img);
					filter->AddInput(&con);
					for (int i = 0; i < inputImages.size(); ++i)
						filter->AddInput(inputImages[i]);
				}
				else
				{
					filterParams["File name"] = fileName;
				}
			}
			filter->Run(filterParams);
			if (curLine == 0)
			{
				QStringList captions;
				if (parameters["Add filename"].toBool())
					captions << "filename";
				for (auto outValue : filter->OutputValues())
				{
					QString curCap(outValue.first);
					curCap.replace(",", "");
					captions << curCap;
				}
				if (outputBuffer.empty())
					outputBuffer.append("");
				outputBuffer[0] += (outputBuffer[0].isEmpty() || captions.empty() ? "" : ",") + captions.join(",");
				++curLine;
			}
			if (curLine >= outputBuffer.size())
				outputBuffer.append("");
			QStringList values;
			QString relFileName = MakeRelative(batchDir, fileName);
			if (parameters["Add filename"].toBool())
			{
				values << relFileName;
			}
			for (auto outValue : filter->OutputValues())
				values.append(outValue.second.toString());
			QString textToAdd = (outputBuffer[curLine].isEmpty() || values.empty() ? "" : ",") + values.join(",");
			outputBuffer[curLine] += textToAdd;
			++curLine;
			for (int o = 0; o < filter->Output().size(); ++o)
			{
				QFileInfo fi(outDir + "/" + relFileName);
				QString multiFileSuffix = filter->Output().size() > 1 ? QString::number(o) : "";
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
					AddMsg(QString("Error creating output directory %1, skipping writing output file %2")
						.arg(fi.absolutePath()).arg(outName));
				else
					iAITKIO::writeFile(outName, filter->Output()[o]->GetITKImage(),
						filter->Output()[o]->GetITKScalarPixelType(), useCompression);
			}
		}
		catch (std::exception & e)
		{
			DEBUG_LOG(QString("Batch processing: Error while processing file '%1': %2").arg(fileName).arg(e.what()));
			if (!parameters["Continue on error"].toBool())
				throw e;
		}
		Progress()->EmitProgress( static_cast<int>(100 * (curLine - 1.0) / files.size()) );
	}

	if (!outputFile.isEmpty())
	{
		QFile file(outputFile);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
			for (QString line : outputBuffer)
			{
				textStream << line << endl;
			}
			file.close();
		}
	}
}

IAFILTER_CREATE(iABatchFilter);
