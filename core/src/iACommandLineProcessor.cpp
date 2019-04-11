/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iACommandLineProcessor.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAMathUtility.h"
#include "iAModuleDispatcher.h"
#include "iAProgress.h"
#include "iAStringHelper.h"
#include "iAValueType.h"
#include "io/iAITKIO.h"

#include <QFileInfo>
#include <QTextStream>

#include <iostream>

iACommandLineProgressIndicator::iACommandLineProgressIndicator(int numberOfSteps, bool quiet) :
	m_numberOfDots(clamp(1, 100, numberOfSteps)),
	m_lastDots(0),
	m_quiet(quiet)
{
	if (!quiet)
	{	// print progress bar "borders"
		std::cout << "|" << QString(" ").repeated(numberOfSteps).toStdString() << "|" << std::endl << " ";
	}
}

void iACommandLineProgressIndicator::Progress(int percent)
{
	if (m_quiet)
		return;
	int curDots = percent * m_numberOfDots / 100;
	if (curDots > m_lastDots)
	{
		QString dot(".");
		std::cout << dot.repeated(curDots - m_lastDots).toStdString();
		if (curDots == m_numberOfDots)
		{
			std::cout << std::endl;
		}
		m_lastDots = curDots;
	}
}

namespace
{
	QString AbbreviateDesc(QString desc)
	{
		int brpos = desc.indexOf("<br/>");
		return brpos != -1 ? desc.left(brpos) : desc;
	}

	void PrintListOfAvailableFilters()
	{
		auto filterFactories = iAFilterRegistry::filterFactories();
		std::cout << "Available filters:" << std::endl;
		for (auto factory : filterFactories)
		{
			auto filter = factory->create();
			std::cout << filter->name().toStdString() << std::endl
				<< "        " << stripHTML(AbbreviateDesc(filter->description())).toStdString() << std::endl << std::endl;
		}
	}

	void PrintFilterHelp(QString filterName)
	{
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << "For a full list of all available filters, execute 'open_iA_cmd -l'" << std::endl;
			return;
		}
		std::cout << filter->name().toStdString() << ":" << std::endl
			<< stripHTML(filter->description().replace("<br/>", "\n")).toStdString() << std::endl;
		std::cout << "Parameters:" << std::endl;
		for (auto p : filter->parameters())
		{
			std::cout << "    " << p->name().toStdString() << " " << ValueType2Str(p->valueType()).toStdString();
			switch (p->valueType())
			{
			case Continuous:
			case Discrete:
				if (p->min() != std::numeric_limits<double>::lowest())
				{
					std::cout << " min=" << p->min();
				}
				if (p->max() != std::numeric_limits<double>::max())
				{
					std::cout << " max=" << p->max();
				}
			case Boolean:		// intentional fall-through!
				std::cout << " default=" << p->defaultValue().toString().toStdString();
				break;
			case Categorical:
				std::cout << " possible values=(" << p->defaultValue().toStringList().join(",").toStdString() << ")";
				break;
			case FileNameOpen:
				std::cout << " specify an existing file.";
				break;
			case FileNamesOpen:
				std::cout << " specify a list of existing filenames.";
				break;
			case FileNameSave:
				std::cout << " specify an output filename.";
				break;
			case Folder:
				std::cout << " specify a folder.";
				break;
			case String: // intentional fall-through!
			case Text:
				std::cout << " text, see filter description for details.";
				break;
			case FilterName:
				std::cout << " name of another filter.";
				break;
			case FilterParameters:
				std::cout << " parameters of a filter.";
				break;
			default: // no more help text available
				break;
			}
			std::cout << std::endl;
		}
	}

	void PrintParameterDescriptor(QString filterName)
	{
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << "For a full list of all available filters, execute 'open_iA_cmd -l'" << std::endl;
			return;
		}
		std::cout << filter->name().toStdString() << ":" << std::endl;
		for (auto p : filter->parameters())
		{
			std::cout << p->name().toStdString() << "\tParameter\t"
					<< ValueType2Str(p->valueType()).toStdString() << "\t";
			if (p->valueType() == Continuous || p->valueType() == Discrete)
				std::cout << p->min() << "\t" << p->max() << "\tLinear";
			else if (p->valueType() == Categorical)
				std::cout << "\t" << p->defaultValue().toStringList().join(",").toStdString();
			std::cout << std::endl;
		}
	}

	void PrintUsage(const char * version)
	{
		std::cout << "open_iA command line tool, version " << version << "." << std::endl
			<< "Usage:" << std::endl
			<< "  > open_iA_cmd (-l|-h ...|-r ...|-p ...)" << std::endl
			<< "Options:" << std::endl
			<< "     -l" << std::endl
			<< "         List available filters" << std::endl
			<< "     -h FilterName" << std::endl
			<< "         Print help on a specific filter" << std::endl
			<< "     -r FilterName -i Input -o Output -p Parameters [-q] [-c] [-f] [-s n]" << std::endl
			<< "         Run the filter given by FilterName with Parameters on given Input, write to Output" << std::endl
			<< "           -q   quiet - no output except for error messages" << std::endl
			<< "           -c   compress output" << std::endl
			<< "           -f   overwrite output if it exists" << std::endl
			<< "           -s n separate input starts at nth filename given under -i" << std::endl // (required for some filters, e.g. Extended Random Walker)
			<< "         Note: Only image output is written to the filename(s) specified after -o," << std::endl
			<< "           filters returning one or more output values write those values to the command line." << std::endl
			<< "     -p FilterName" << std::endl
			<< "         Output the Parameter Descriptor for the given filter (required for sampling)." << std::endl;
	}

	enum ParseMode { None, Input, Output, Parameter, InvalidParameter, Quiet, Compress, Overwrite, InputSeparation};

	ParseMode GetMode(QString arg)
	{
		if (arg == "-i") return Input;
		else if (arg == "-o") return Output;
		else if (arg == "-p") return Parameter;
		else if (arg == "-q") return Quiet;
		else if (arg == "-c") return Compress;
		else if (arg == "-f") return Overwrite;
		else if (arg == "-s") return InputSeparation;
		else return InvalidParameter;
	}

	int RunFilter(QStringList const & args)
	{
		QString filterName = args[0];
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << QString("Filter '%1' does not exist!").arg(filterName).toStdString() << std::endl
				<< "For a full list of all available filters, execute 'open_iA_cmd -l'" << std::endl;
			return 1;
		}
		QStringList inputFiles;
		QStringList outputFiles;
		QMap<QString, QVariant> parameters;
		bool quiet = false;
		bool compress = false;
		bool overwrite = false;
		int mode = None;
		for (int a = 1; a < args.size(); ++a)
		{
			switch (mode)
			{
			case None:
			case Quiet:
			case Compress:
			case Overwrite:
				mode = GetMode(args[a]);
				break;
			case InputSeparation: {
				bool ok;
				int inputSeparation = args[a].toInt(&ok);
				if (!ok)
				{
					std::cout << "Invalid value '" << args[a].toStdString()
						<< "' for input separation, expected a int!" << std::endl;
					return 1;
				}
				filter->setFirstInputChannels(inputSeparation);
				mode = None;
				break;
			}
			case Input:
			case Output:
			case Parameter:
				if (args[a].startsWith("-") &&
					(mode != Parameter || parameters.size() >= filter->parameters().size()))
				{
					mode = GetMode(args[a]);
				}
				else
				{
					switch (mode)
					{
					case Input:     inputFiles << args[a];  break;
					case Output:    outputFiles << args[a]; break;
					case Parameter:
						{
							int paramIdx = parameters.size();
							if (paramIdx >= filter->parameters().size())
							{
								std::cout << QString("More parameters (%1) given than expected(%2)!")
									.arg(paramIdx + 1).arg(filter->parameters().size()).toStdString() << std::endl;
							}
							else
							{
								QString paramName = filter->parameters()[paramIdx]->name();
								QString value(args[a]);
								if (filter->parameters()[paramIdx]->valueType() == Text)
								{
									QFile f(value);
									if (!f.open(QFile::ReadOnly | QFile::Text))
									{
										std::cout << QString("Expected a filename as input for text parameter '%1', but could not open '%2' as a text file.")
											.arg(paramName).arg(value).toStdString() << std::endl;
										return 1;
									}
									QTextStream in(&f);
									value = in.readAll();
								}
								parameters.insert(paramName, value);
							}
						}
					}
				}
				break;
			}
			if (mode == InvalidParameter)
			{
				std::cout << QString("Invalid/Unexpected parameter: '%1', please check your syntax!").arg(args[a]).toStdString() << std::endl;
				return 1;
			}
			if (mode == Quiet)
			{
				quiet = true;
			}
			if (mode == Compress)
			{
				compress = true;
			}
			if (mode == Overwrite)
			{
				overwrite = true;
			}
		}

		// Argument checks:
		if (inputFiles.size() == 0)
		{
			std::cout << "Missing input files - please specify at least one after the -i parameter" << std::endl;
			return 1;
		}
		if (outputFiles.size() < filter->outputCount())
		{
			std::cout << "Missing output files - please specify at least one after the -o parameter" << std::endl;
			return 1;
		}
		if (parameters.size() != filter->parameters().size())
		{
			std::cout << QString("Invalid number of parameters: %2 expected, %1 were given.")
				.arg(parameters.size())
				.arg(filter->parameters().size()).toStdString()
				<< std::endl;
			return 1;
		}

		try
		{
			for (int i = 0; i < inputFiles.size(); ++i)
			{
				if (!quiet)
				{
					std::cout << "Reading input file '" << inputFiles[i].toStdString() << "'" << std::endl;
				}
				iAITKIO::ScalarPixelType pixelType;
				iAITKIO::ImagePointer img = iAITKIO::readFile(inputFiles[i], pixelType, false);
				iAConnector * con = new iAConnector();
				con->setImage(img);
				filter->addInput(con);
			}

			if (!quiet)
			{
				std::cout << "Running filter '" << filter->name().toStdString() << "' with parameters: " << std::endl;
				for (int p = 0; p < parameters.size(); ++p)
				{
					std::cout << "    " << filter->parameters()[p]->name().toStdString()
						<< "=" << parameters[filter->parameters()[p]->name()].toString().toStdString() << std::endl;
				}
			}
			iAProgress progress;
			iACommandLineProgressIndicator progressIndicator(50, quiet);
			QObject::connect(&progress, SIGNAL(progress(int)), &progressIndicator, SLOT(Progress(int)));
			filter->setProgress(&progress);
			if (!filter->checkParameters(parameters))
			{   // output already happened in CheckParameters via logger
				return 1;
			}
			if (!filter->run(parameters))
			{	// output already happened in Run via logger
				return 1;
			}
			// write output file(s)
			for (int o = 0; o < filter->output().size(); ++o)
			{
				QString outFileName;
				if (filter->output().size() == 1 ||  o < outputFiles.size()-1)
				{
					outFileName = outputFiles[o];
				}
				else
				{
					QFileInfo fi(outputFiles[outputFiles.size() - 1]);
					outFileName = QString("%1/%2%3.%4").arg(fi.absolutePath()).arg(fi.baseName())
						.arg(o-outputFiles.size()+1).arg(fi.completeSuffix());
				}
				if (QFile(outFileName).exists() && !overwrite)
				{
					// TODO: check at beginning to avoid aborting after long operation? But output count might not be known then...
					std::cout << QString("Output file '%1' already exists! Aborting. "
						"Specify -f to overwrite existing files.").arg(outFileName).toStdString();
					return 1;
				}
				if (!quiet)
				{
					std::cout << QString("Writing output %1 to file: '%2' (compression: %3)")
						.arg(o).arg(outFileName).arg(compress ? "on" : "off").toStdString()
						<< std::endl;
				}
				iAITKIO::writeFile(outFileName, filter->output()[o]->itkImage(), filter->output()[o]->itkScalarPixelType(), compress);
			}
			for (auto outputValue: filter->outputValues())
				std::cout << outputValue.first.toStdString() << ": "
					<< outputValue.second.toString().toStdString() << std::endl;

			return 0;
		}
		catch (std::exception & e)
		{
			std::cout << "ERROR: " << e.what() << std::endl;
			return 1;
		}
	}
}

int ProcessCommandLine(int argc, char const * const * argv, const char * version)
{
	auto dispatcher = new iAModuleDispatcher(QFileInfo(argv[0]).absolutePath());
	dispatcher->InitializeModules(iAStdOutLogger::get());
	if (argc > 1 && QString(argv[1]) == "-l")
	{
		PrintListOfAvailableFilters();
	}
	else if (argc > 2 && QString(argv[1]) == "-h")
	{
		PrintFilterHelp(argv[2]);
	}
	else if (argc > 2 && QString(argv[1]) == "-r")
	{
		QStringList args;
		for (int a = 2; a < argc; ++a)
		{
			args << argv[a];
		}
		return RunFilter(args);
	}
	else if (argc > 2 && QString(argv[1]) == "-p")
	{
		PrintParameterDescriptor(argv[2]);
	}
	else
	{
		PrintUsage(version);
	}
	return 0;
}
