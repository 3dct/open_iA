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

void iACommandLineProgressIndicator::progress(int percent)
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
	QString abbreviateDesc(QString desc)
	{
		auto brpos = desc.indexOf("<br/>");
		return brpos != -1 ? desc.left(brpos) : desc;
	}

	void printAttributes(iAAttributes const & attr)
	{
		if (attr.isEmpty())
		{
			std::cout << "No parameters\n";
		}
		for (auto p : attr)
		{
			std::cout << p->name().toStdString() << "\tParameter\t" << ValueType2Str(p->valueType()).toStdString()
					  << "\t";
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

	void printListOfAvailableFilters(QString sortBy)
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
				filter->name().toStdString(), stripHTML(abbreviateDesc(filter->description())).toStdString()));
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

	void printFilterHelp(QString filterName)
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

	void printParameterDescriptor(QString filterName)
	{
		auto filter = iAFilterRegistry::filter(filterName);
		if (!filter)
		{
			std::cout << "For a full list of all available filters, execute 'open_iA_cmd -l'\n";
			return;
		}
		std::cout << filter->name().toStdString() << ":\n";
		printAttributes(filter->parameters());
	}

	void printFormatInfo(QString extension)
	{
		if (extension[0] != '.')
		{
			extension = "." + extension;
		}
		auto io = iAFileTypeRegistry::createIO("test" + extension, iAFileIO::Load);
		std::cout << "Parameters for loading a file with extension " << extension.toStdString() << ":\n";
		if (!io)
		{
			std::cout << "    Not a supported file format!\n";
			return;
		}
		auto inAttr(io->parameter(iAFileIO::Load));
		removeAttribute(inAttr, iADataSet::FileNameKey);
		printAttributes(inAttr);
		std::cout << "Parameters for saving a file with extension " << extension.toStdString() << "\n";
		auto outAttr(io->parameter(iAFileIO::Save));
		printAttributes(outAttr);
	}

	std::string prepFormatForPrint(QString const& f)
	{
		const QString Separator = "\n  - ";
		auto formats = f.split(";;", Qt::SkipEmptyParts);  // skip empty (at end)
		formats.remove(0, 1);                              // first is the "any supported" collection ...
		return (Separator + formats.join(Separator)).toStdString();
	}

	void printFormats()
	{
		std::cout << "Formats available for loading:" <<
			prepFormatForPrint(iAFileTypeRegistry::registeredFileTypes(iAFileIO::Load)) << "\n\n";
		std::cout << "Formats available for saving:" <<
			prepFormatForPrint(iAFileTypeRegistry::registeredFileTypes(iAFileIO::Save)) << "\n";
	}

	void printUsage(const char * version)
	{
		std::cout << "open_iA command line tool, version " << version << ".\n"
			<< "Usage:\n"
			<< "  > open_iA_cmd command [options]\n"
			<< "With command one of:\n"
			<< "     list [name|category|fullCategory]\n"
			<< "         List available filters, sorted by name (default) or by category\n"
			<< "     help FilterName\n"
			<< "         Print help on a specific filter\n"
			<< "     run FilterName -i Input -o Output -p Parameters [-q] [-c] [-f] [-s n] [-j InParams] [-k OutParams]\n"
			<< "         Run the filter given by FilterName with Parameters on given Input, write to Output\n"
			<< "           -i   the list of input filenames. If a filename contains one or more spaces, it needs\n"
			<< "                to be quoted, e.g. \"my input file.mhd\"\n"
			<< "           -o   the name of the file(s) to which the output of the filter should be written.\n"
			<< "                Same as for input files, if the file name contains spaces, it needs to be quoted.\n"
			<< "           -p   the parameters of the filter to be run. Use 'parameters' command to determine the\n"
			<< "                parameters expected for a given filter.\n"
			<< "                If a parameter value contains spaces (for example certain strings in categorical type\n"
			<< "                parameters), it needs to be surrounded with quotes.\n"
			<< "                Vector type parameters need to be specified as a quoted list separated by commas,\n"
			<< "                e.g. \"1.0,0.0,0.0\" for a vector of 3 double values.\n"
			<< "                If the parameter type says 'Text', then a file name is expected here; the file\n"
			<< "                content is read from disk and passed to the filter\n"
			<< "                For Boolean type parameters, specify 'true', 'on', 'yes' or any uppercase variant\n"
			<< "                of these words to signify a value of true; every other value will be taken as\n"
			<< "                meaning false.\n"
			<< "           -q   quiet - no output except for error messages\n"
			<< "           -f   overwrite output if it exists\n"
			<< "           -s n separate input starts at nth filename given under -i\n"
			<< "                (required for some filters, e.g.Extended Random Walker)\n"
			<< "           -v n specify the log level (how verbose output should be).\n"
			<< "                Can be " << AvailableLogLevels().join(", ").toStdString()
			                    << " or a numeric value (" << lvlDebug << ".." << lvlFatal << ")\n"
			<< "                between 1 and 5 (1=DEBUG, ...). Default is WARN.\n"
			<< "           -j InputParameters for input file formats that require parameters (e.g. raw files)\n"
			<< "                Note that -i with all input filenames needs to be specified before -j for the\n"
			<< "                program to be able to determine the input parameters necessary for the given files.\n"
			<< "                Specify -j once per input file that requires parameters.\n"
			<< "                You can check whether a file format requires input parameters through the\n"
			<< "                'formatinfo' command (see below).\n"
			<< "                The parameters need to be specified analogously to the -p option, see notes there.\n"
			<< "           -k OutputParameters for output file formats that take parameters (e.g. compress for .mhd)\n"
			<< "                Note that -o with all output filenames needs to be specified before -k for the\n"
			<< "                program to be able to determine the input parameters necessary for the given files.\n"
			<< "                Specify -k once per filename that requires parameters.\n"
			<< "                You can check whether a file format requires input parameters through the\n"
			<< "                'formatinfo' command (see below).\n"
			<< "                The parameters need to be specified analogously to the -p option, see notes there.\n"
			<< "         Note: Only image and mesh output is written to the filename(s) specified after -o,\n"
			<< "           filters returning one or more output values write those values to the command line.\n"
			<< "     parameters FilterName\n"
			<< "         Output the Parameter Descriptor for the given filter (required for sampling).\n"
			<< "     formatinfo Extension\n"
			<< "         Output information on which parameters a file format with the given extension\n"
			<< "         (can be specified with or without leading '.') has for loading/saving.\n";
	}

	enum iAParseMode { None, Input, Output, Parameter, InvalidParameter, Quiet, Overwrite, InputSeparation, LogLevel, InputParameters, OutputParameters };

	iAParseMode getMode(QString arg)
	{
		if (arg == "-i") return Input;
		else if (arg == "-o") return Output;
		else if (arg == "-p") return Parameter;
		else if (arg == "-q") return Quiet;
		else if (arg == "-f") return Overwrite;
		else if (arg == "-s") return InputSeparation;
		else if (arg == "-v") return LogLevel;
		else if (arg == "-k") return OutputParameters;
		else if (arg == "-j") return InputParameters;
		else return InvalidParameter;
	}

	bool addParameterValue(QVariantMap & parameters, iAAttributes const & inAttr, QString curValue, bool removeFileKey)
	{
		auto paramIdx = parameters.size();
		auto attr(inAttr);
		if (removeFileKey)
		{
			removeAttribute(attr, iADataSet::FileNameKey);
		}
		if (paramIdx >= attr.size())
		{
			std::cout << QString("More parameters (%1) given than expected(%2)!")
				.arg(paramIdx + 1).arg(attr.size()).toStdString() << std::endl;
		}
		else
		{
			QString paramName = attr[paramIdx]->name();
			QVariant value(curValue);
			if (attr[paramIdx]->valueType() == iAValueType::Text)
			{
				QFile f(curValue);
				if (!f.open(QFile::ReadOnly | QFile::Text))
				{
					std::cout << QString("Expected a filename as input for text parameter '%1', but could not open '%2' as a text file.")
						.arg(paramName).arg(curValue).toStdString() << std::endl;
					return false;
				}
				QTextStream in(&f);
				value = in.readAll();
			}
			else if (attr[paramIdx]->valueType() == iAValueType::Boolean)
			{
				curValue = curValue.toLower();
				value = curValue == "true" || curValue == "yes" || curValue == "on";
			}
			parameters.insert(paramName, value);
		}
		return true;
	}

	void getNextIdxIO(QStringList const& inputFiles, qsizetype& nextIdx, std::shared_ptr<iAFileIO>& io, iAFileIO::Operation ioType)
	{
		while (nextIdx < inputFiles.size())
		{
			io = iAFileTypeRegistry::createIO(inputFiles[nextIdx], ioType);
			auto param = io->parameter(ioType);
			qsizetype minParams = findAttribute(param, iADataSet::FileNameKey) == -1 ? 0 : 1;
			if (param.size() > minParams)
			{
				break;
			}
			++nextIdx;
		}
	}

	int runFilter(QStringList const & args)
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
		QVector<QVariantMap> inParams;
		QVector<QVariantMap> outParams;
		bool quiet = false;
		bool overwrite = false;
		int mode = None;
		qsizetype curInIdx = 0;
		qsizetype curOutIdx = 0;
		std::shared_ptr<iAFileIO> curInIO = nullptr;
		std::shared_ptr<iAFileIO> curOutIO = nullptr;
		// TODO: use command line parsing library instead!
		for (int a = 1; a < args.size(); ++a)
		{
			switch (mode)
			{
			case None:
			case Quiet:
			case Overwrite:
				mode = getMode(args[a]);
				break;
			case InputSeparation:
			{
				bool ok;
				int inputSeparation = args[a].toInt(&ok);
				if (!ok || inputSeparation <= 0)
				{
					std::cout << "Invalid value '" << args[a].toStdString()
					          << "' for input separation, expected an integer number >= 1!\n";
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
						          << "' for log level, expected an integer number between 1 and 5!\n";
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
			case InputParameters:
			case OutputParameters:
				if (args[a].startsWith("-") &&
					(mode != Parameter || parameters.size() >= filter->parameters().size()))
				{
					if (mode == Input || mode == InputParameters)
					{
						if (mode == InputParameters)
						{
							++curInIdx;
						}
						else
						{
							inParams = QVector<QVariantMap>(inputFiles.size());
						}
						getNextIdxIO(inputFiles, curInIdx, curInIO, iAFileIO::Load);
					}
					else if (mode == Output || mode == OutputParameters)
					{
						if (mode == OutputParameters)
						{
							++curOutIdx;
						}
						else
						{
							outParams = QVector<QVariantMap>(outputFiles.size());
						}
						getNextIdxIO(outputFiles, curOutIdx, curOutIO, iAFileIO::Save);
					}
					mode = getMode(args[a]);
				}
				else
				{
					switch (mode)
					{
					case Input:     inputFiles << args[a];  break;
					case Output:    outputFiles << args[a]; break;
					case Parameter:        addParameterValue(parameters  , filter->parameters() , args[a], false); break;
					case InputParameters:
						if (curInIO && curInIdx < inParams.size())
						{
							addParameterValue(inParams[curInIdx], curInIO->parameter(iAFileIO::Load), args[a], true); 
						}
						else
						{
							std::cout << QString("Invalid/Unexpected input parameters at position %1 (value %2) - Ignored!\n").arg(a).arg(args[a]).toStdString();
						}
						break;
					case OutputParameters:

						if (curOutIO && curOutIdx < outParams.size())
						{
							addParameterValue(outParams[curOutIdx], curOutIO->parameter(iAFileIO::Save), args[a], true);
						}
						else
						{
							std::cout << QString("Invalid/Unexpected output parameters %1 (value %2) - Ignored!\n").arg(a).arg(args[a]).toStdString();
						}
						break;
					}
				}
				break;
			}
			if (mode == InvalidParameter)
			{
				std::cout << QString("Invalid/Unexpected parameter: '%1', please check your syntax!").arg(args[a]).toStdString() << "\n";
				return 1;
			}
			if (mode == Quiet)
			{
				quiet = true;
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
				// TODO: use progress indicator
				auto dataSet = io->load(inputFiles[i], inParams[i]);
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
			QObject::connect(filter->progress(), &iAProgress::progress, &progressIndicator, &iACommandLineProgressIndicator::progress);
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
					std::cout << QString("Writing output %1 to file: '%2'").arg(o).arg(outFileName).toStdString()
						<< std::endl;
				}

				auto io = iAFileTypeRegistry::createIO(outFileName, iAFileIO::Save);
				if (!io)
				{
					std::cout << QString("Could not find a writer suitable for file name %1!")
						.arg(outFileName).toStdString() << std::endl;
					return 1;
				}
				// TODO: use progress indicator here
				io->save(outFileName, filter->outputs()[o], outParams[o]);
			}
#if QT_VERSION >= QT_VERSION_CHECK(6,4,0)
			for (auto [name, value] : filter->outputValues().asKeyValueRange())
			{
#else
			for (auto it = map.keyValueBegin(); it != map.keyValueEnd(); ++it)
			{
				auto name = it->first;
				auto value = it->second;
#endif
				std::cout << name.toStdString() << ": " << value.toString().toStdString() << std::endl;
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

int processCommandLine(int argc, char const * const * argv, const char * version)
{
	auto dispatcher = new iAModuleDispatcher(QFileInfo(argv[0]).absolutePath());
	dispatcher->InitializeModules();
	if (argc > 1 && QString(argv[1]).toLower() == "list")
	{
		printListOfAvailableFilters(argc > 2 ? QString(argv[2]) : QString("name") );
	}
	else if (argc > 2 && QString(argv[1]).toLower() == "help")
	{
		printFilterHelp(argv[2]);
	}
	else if (argc > 2 && QString(argv[1]).toLower() == "run")
	{
		QStringList args;
		for (int a = 2; a < argc; ++a)
		{
			args << argv[a];
		}
		return runFilter(args);
	}
	else if (argc > 2 && QString(argv[1]).toLower() == "parameters")
	{
		printParameterDescriptor(argv[2]);
	}
	else if (argc > 2 && QString(argv[1]).toLower() == "formatinfo")
	{
		printFormatInfo(argv[2]);
	}
	else if (argc > 1 && QString(argv[1]).toLower() == "formats")
	{
		printFormats();
	}
	else
	{
		printUsage(version);
	}
	return 0;
}
