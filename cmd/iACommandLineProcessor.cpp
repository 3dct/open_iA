// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACommandLineProcessor.h"

#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iALoggerStdOut.h"

// io
#include "iAFileTypeRegistry.h"

#include "iAAttributeDescriptor.h"
#include "iALogLevelMappings.h"
#include "iAMathUtility.h"
#include "iAModuleDispatcher.h"
#include "iAProgress.h"
#include "iAStringHelper.h"
#include "iAValueType.h"

#include <QFileInfo>
#include <QTextStream>

#include <iostream>
#include <map>

iACommandLineProgressIndicator::iACommandLineProgressIndicator(int numberOfSteps, bool quiet) :
	m_lastDots(0),
	m_numberOfDots(clamp(1, 100, numberOfSteps)),
	m_quiet(quiet)
{
	if (!quiet)
	{	// print progress bar "borders"
		std::cout << "|" << QString(" ").repeated(numberOfSteps).toStdString() << "|\n " << std::flush;
	}
}

void iACommandLineProgressIndicator::Progress(int percent)
{
	if (m_quiet)
	{
		return;
	}
	int curDots = percent * m_numberOfDots / 100;
	if (curDots > m_lastDots)
	{
		QString dot(".");
		std::cout << dot.repeated(curDots - m_lastDots).toStdString();
		if (curDots == m_numberOfDots)
		{
			std::cout << "\n";
		}
		std::cout << std::flush;
		m_lastDots = curDots;
	}
}

namespace
{
	QString AbbreviateDesc(QString desc)
	{
		auto brpos = desc.indexOf("<br/>");
		return brpos != -1 ? desc.left(brpos) : desc;
	}

	void PrintListOfAvailableFilters(QString sortBy)
	{
		auto filterFactories = iAFilterRegistry::filterFactories();
		std::cout << "Available filters:\n";
		std::map<std::string, std::map<std::string, std::string>> filterMap;
		bool sortByCat = sortBy == "category" || sortBy == "fullCategory";
		for (auto factory : filterFactories)
		{
			auto filter = factory();
			std::string category = (sortBy == "category") ? filter->category().toStdString() :
				((sortBy == "fullCategory") ? filter->fullCategory().toStdString() : "");
			filterMap[category].insert(std::make_pair(
				filter->name().toStdString(), stripHTML(AbbreviateDesc(filter->description())).toStdString()));
		}
		for (auto c: filterMap)
		{
			if (sortByCat)
			{
				std::cout << (c.first.empty() ? "Uncategorized" : c.first) << ":\n";
			}
			for (auto f : c.second)
			{
				std::string prefix(sortByCat ? 4 : 0, ' ');
				std::cout << prefix << f.first << "\n" << prefix << "    " << f.second << "\n";
			}
		}
	}

	void PrintFilterHelp(QString filterName)
	{
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << "For a full list of all available filters, execute 'open_iA_cmd -l'\n";
			return;
		}
		std::cout << filter->name().toStdString() << ":\n"
		          << stripHTML(filter->description().replace("<br/>", "\n")).toStdString() << "\n"
		          << "Parameters:\n";
		for (auto p : filter->parameters())
		{
			std::cout << "    " << p->name().toStdString() << " " << ValueType2Str(p->valueType()).toStdString();
			switch (p->valueType())
			{
			case iAValueType::Continuous:
			case iAValueType::Discrete:
				if (p->min() != std::numeric_limits<double>::lowest())
				{
					std::cout << " min=" << p->min();
				}
				if (p->max() != std::numeric_limits<double>::max())
				{
					std::cout << " max=" << p->max();
				}
				[[fallthrough]];
			case iAValueType::Boolean:
				std::cout << " default=" << p->defaultValue().toString().toStdString();
				break;
			case iAValueType::Categorical:
				std::cout << " possible values=(" << p->defaultValue().toStringList().join(",").toStdString() << ")";
				break;
			case iAValueType::FileNameOpen:
				std::cout << " specify an existing file.";
				break;
			case iAValueType::FileNamesOpen:
				std::cout << " specify a list of existing filenames.";
				break;
			case iAValueType::FileNameSave:
				std::cout << " specify an output filename.";
				break;
			case iAValueType::Folder:
				std::cout << " specify a folder.";
				break;
			case iAValueType::String:
				[[fallthrough]];
			case iAValueType::Text:
				std::cout << " text, see filter description for details.";
				break;
			case iAValueType::FilterName:
				std::cout << " name of another filter.";
				break;
			case iAValueType::FilterParameters:
				std::cout << " parameters of a filter.";
				break;
			default: // no more help text available
				break;
			}
			std::cout << "\n";
		}
		if (filter->requiredImages() == 0)
		{
			std::cout << "No input images.\n";
		}
		else
		{
			std::cout << "Input images:\n";
			for (unsigned int  i = 0; i < filter->requiredImages(); ++i)
			{
				std::cout << "    " << filter->inputName(i).toStdString() << "\n";
			}
		}
		if (filter->plannedOutputCount() == 0)
		{
			std::cout << "No output images.\n";
		}
		else
		{
			std::cout << "Output images:\n";
			for (unsigned int i = 0; i < filter->plannedOutputCount(); ++i)
			{
				std::cout << "    " << filter->outputName(i).toStdString() << "\n";
			}
		}
	}

	void PrintParameterDescriptor(QString filterName)
	{
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << "For a full list of all available filters, execute 'open_iA_cmd -l'\n";
			return;
		}
		std::cout << filter->name().toStdString() << ":\n";
		for (auto p : filter->parameters())
		{
			std::cout << p->name().toStdString() << "\tParameter\t"
			          << ValueType2Str(p->valueType()).toStdString() << "\t";
			if (p->valueType() == iAValueType::Continuous || p->valueType() == iAValueType::Discrete)
			{
				std::cout << p->min() << "\t" << p->max() << "\tLinear";
			}
			else if (p->valueType() == iAValueType::Categorical)
			{
				std::cout << "\t" << p->defaultValue().toStringList().join(",").toStdString();
			}
			std::cout << "\n";
		}
	}

	void PrintUsage(const char * version)
	{
		std::cout << "open_iA command line tool, version " << version << ".\n"
			<< "Usage:\n"
			<< "  > open_iA_cmd (-l [...]|-h ...|-r ...|-p ...)\n"
			<< "Options:\n"
			<< "     -l [name|category|fullCategory]\n"
			<< "         List available filters, sorted by name (default) or by category\n"
			<< "     -h FilterName\n"
			<< "         Print help on a specific filter\n"
			<< "     -r FilterName -i Input -o Output -p Parameters [-q] [-c] [-f] [-s n]\n"
			<< "         Run the filter given by FilterName with Parameters on given Input, write to Output\n"
			<< "           -q   quiet - no output except for error messages\n"
			<< "           -c   compress output\n"
			<< "           -f   overwrite output if it exists\n"
			<< "           -s n separate input starts at nth filename given under -i\n" // (required for some filters, e.g. Extended Random Walker)
			<< "           -v n specify the log level (how verbose output should be).\n"
			<< "                Can be " << AvailableLogLevels().join(", ").toStdString()
			                             << " or a numeric value (" << lvlDebug << ".." << lvlFatal << ")\n"
			<< "                between 1 and 5 (1=DEBUG, ...). Default is WARN.\n"
			<< "         Note: Only image output is written to the filename(s) specified after -o,\n"
			<< "           filters returning one or more output values write those values to the command line.\n"
			<< "     -p FilterName\n"
			<< "         Output the Parameter Descriptor for the given filter (required for sampling).\n";
	}

	enum ParseMode { None, Input, Output, Parameter, InvalidParameter, Quiet, Compress, Overwrite, InputSeparation, LogLevel };

	ParseMode GetMode(QString arg)
	{
		if (arg == "-i") return Input;
		else if (arg == "-o") return Output;
		else if (arg == "-p") return Parameter;
		else if (arg == "-q") return Quiet;
		else if (arg == "-c") return Compress;
		else if (arg == "-f") return Overwrite;
		else if (arg == "-s") return InputSeparation;
		else if (arg == "-v") return LogLevel;
		else return InvalidParameter;
	}

	int RunFilter(QStringList const & args)
	{
		QString filterName = args[0];
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << QString("Filter '%1' does not exist!").arg(filterName).toStdString() << "\n"
			          << "For a full list of all available filters, execute 'open_iA_cmd -l'\n";
			return 1;
		}
		QStringList inputFiles;
		QStringList outputFiles;
		QVariantMap parameters;
		bool quiet = false;
		bool compress = false;
		bool overwrite = false;
		int mode = None;
		// TODO: use command line parsing library instead!
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
			case InputSeparation:
			{
				bool ok;
				int inputSeparation = args[a].toInt(&ok);
				if (!ok)
				{
					std::cout << "Invalid value '" << args[a].toStdString()
					          << "' for input separation, expected a int!\n";
					return 1;
				}
				filter->setFirstInputChannels(inputSeparation);
				mode = None;
				break;
			}
			case LogLevel:
			{
				bool ok;
				int intLevel = args[a].toInt(&ok);
				if (!ok)
				{
					iALogLevel logLevel = stringToLogLevel(args[a], ok);
					if (!ok)
					{
						std::cout << "Invalid value '" << args[a].toStdString()
						          << "' for log level, expected an int between 1 and 5!\n";
					}
					else
					{
						iALog::get()->setLogLevel(logLevel);
					}
				}
				else
				{
					iALog::get()->setLogLevel(static_cast<iALogLevel>(intLevel));
				}
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
							auto paramIdx = parameters.size();
							if (paramIdx >= filter->parameters().size())
							{
								std::cout << QString("More parameters (%1) given than expected(%2)!")
									.arg(paramIdx + 1).arg(filter->parameters().size()).toStdString() << std::endl;
							}
							else
							{
								QString paramName = filter->parameters()[paramIdx]->name();
								QString value(args[a]);
								if (filter->parameters()[paramIdx]->valueType() == iAValueType::Text)
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
		if (static_cast<unsigned int>(inputFiles.size()) != filter->requiredImages())
		{
			std::cout << "Incorrect number of input files: filter requires "
				<< filter->requiredImages() << " input files, but "
				<< inputFiles.size() << " were specified after the -i parameter." << std::endl;
			return 1;
		}
		if (static_cast<unsigned int>(outputFiles.size()) < filter->plannedOutputCount())
		{
			std::cout << "Missing output files - please specify at least one after the -o parameter" << std::endl;
			return 1;
		}
		if (parameters.size() != filter->parameters().size())
		{
			std::cout << QString("Incorrect number of parameters: %2 expected, %1 were given.")
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
				auto io = iAFileTypeRegistry::createIO(inputFiles[i], iAFileIO::Load);
				if (!io)
				{
					std::cout << QString("Could not find a reader suitable for file name %1!")
						.arg(inputFiles[i]).toStdString() << std::endl;
					return 1;
				}
				QVariantMap dummyParams;  // TODO: allow reading i/o parameters from cmd, use progress indicator
				auto dataSet = io->load(inputFiles[i], dummyParams);
				filter->addInput(dataSet);
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
			iACommandLineProgressIndicator progressIndicator(50, quiet);
			QObject::connect(filter->progress(), &iAProgress::progress, &progressIndicator, &iACommandLineProgressIndicator::Progress);
			if (!filter->checkParameters(parameters))
			{   // output already happened in checkParameters via logger
				return 1;
			}
			if (!filter->run(parameters))
			{	// output already happened in run via logger
				return 1;
			}
			// write output file(s)
			for (size_t o = 0; o < filter->finalOutputCount(); ++o)
			{
				QString outFileName;
				if (filter->finalOutputCount() == 1 || static_cast<qsizetype>(o) < outputFiles.size() - 1)
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

				auto io = iAFileTypeRegistry::createIO(outFileName, iAFileIO::Save);
				if (!io)
				{
					std::cout << QString("Could not find a writer suitable for file name %1!")
						.arg(outFileName).toStdString() << std::endl;
					return 1;
				}
				QVariantMap writeParamValues;
				writeParamValues[iAFileIO::CompressionStr] = compress;
				// TODO: use progress indicator here
				io->save(outFileName, filter->outputs()[o], writeParamValues);
			}
			for (auto outputValue : filter->outputValues())
			{
				std::cout << outputValue.first.toStdString() << ": "
					<< outputValue.second.toString().toStdString() << std::endl;
			}

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
	dispatcher->InitializeModules();
	if (argc > 1 && QString(argv[1]) == "-l")
	{
		PrintListOfAvailableFilters(argc > 2 ? QString(argv[2]) : QString("name") );
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
