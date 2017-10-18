#include "iACommandLineProcessor.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAITKIO.h"
#include "iAMathUtility.h"
#include "iAModuleDispatcher.h"
#include "iAProgress.h"
#include "iAStringHelper.h"
#include "iAValueType.h"

#include "QFileInfo"

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
		auto filterFactories = iAFilterRegistry::FilterFactories();
		std::cout << "Available filters:" << std::endl;
		for (auto factory : filterFactories)
		{
			auto filter = factory->Create();
			std::cout << filter->Name().toStdString() << std::endl
				<< "        " << StripHTML(AbbreviateDesc(filter->Description())).toStdString() << std::endl;
		}
	}

	QSharedPointer<iAFilter> FindFilter(QString filterName)
	{
		auto filterFactories = iAFilterRegistry::FilterFactories();
		for (auto factory : filterFactories)
		{
			auto filter = factory->Create();
			if (filter->Name() == filterName)
			{
				return filter;
			}
		}
		return QSharedPointer<iAFilter>();
	}
	
	void PrintFilterHelp(QString filterName)
	{
		auto filter = FindFilter(filterName);
		if (!filter)
		{
			std::cout << QString("Filter '%1' does not exist!").arg(filterName).toStdString() << std::endl
				<< "For a full list of all available filters, execute 'open_iA_cmd -l'" << std::endl;
			return;
		}
		std::cout << filter->Name().toStdString() << ":" << std::endl
			<< StripHTML(filter->Description().replace("<br/>", "\n")).toStdString() << std::endl;
		std::cout << "Parameters:" << std::endl;
		for (auto p : filter->Parameters())
		{
			std::cout << "    " << p->Name().toStdString() << " " << ValueType2Str(p->ValueType()).toStdString();
			switch (p->ValueType())
			{
			case Continuous:
			case Discrete:
				if (p->Min() != std::numeric_limits<double>::lowest())
				{
					std::cout << " min=" << p->Min();
				}
				if (p->Max() != std::numeric_limits<double>::max())
				{
					std::cout << " max=" << p->Min();
				}
			case Boolean:		// intentional fall-through!
				std::cout << " default=" << p->DefaultValue().toString().toStdString();
				break;
			case Categorical:
				std::cout << " possible values=(" << p->DefaultValue().toStringList().join(",").toStdString() << ")";
			}
			std::cout << std::endl;
		}
	}

	void PrintUsage()
	{
		std::cout << "open_iA command line tool. Usage:" << std::endl
			<< "  open_iA_cmd [-l] [-h ...] [-r ...]" << std::endl
			<< "     -l" << std::endl
			<< "         List available filters" << std::endl
			<< "     -h FilterName" << std::endl
			<< "         Print help on a specific filter" << std::endl
			<< "     -r FilterName -i Input -o Output -p Parameters [-q] [-c]" << std::endl
			<< "         Run the filter given by FilterName with Parameters on given Input, write to Output" << std::endl
			<< "           -q   quiet - no output except for error messages" << std::endl
			<< "           -c   compress output" << std::endl;
	}

	enum ParseMode { None, Input, Output, Parameter, InvalidParameter, Quiet, Compress};

	ParseMode GetMode(QString arg)
	{
		if (arg == "-i") return Input;
		else if (arg == "-o") return Output;
		else if (arg == "-p") return Parameter;
		else if (arg == "-q") return Quiet;
		else if (arg == "-c") return Compress;
		else return InvalidParameter;
	}

	int RunFilter(QStringList const & args)
	{
		QString filterName = args[0];
		auto filter = FindFilter(filterName);
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
		int mode = None;
		for (int a = 1; a < args.size(); ++a)
		{
			switch (mode)
			{
			case None:
			case Quiet:
			case Compress:
				mode = GetMode(args[a]);
				break;
			case Input:
			case Output:
			case Parameter:
				if (args[a].startsWith("-"))
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
							QString paramName = filter->Parameters()[paramIdx]->Name();
							parameters.insert(paramName, args[a]);
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
		}

		// Argument checks:
		if (inputFiles.size() == 0)
		{
			std::cout << "Missing input files - please specify at least one after the -i parameter" << std::endl;
			return 1;
		}
		else if (inputFiles.size() > 1)
		{
			std::cout << "WARNING: More than one input file specified - this is not yet supported!" << std::endl;
		}
		if (outputFiles.size() == 0)
		{
			std::cout << "Missing output files - please specify at least one after the -o parameter" << std::endl;
			return 1;
		}
		else if (outputFiles.size() > 1)
		{
			std::cout << "WARNING: More than one output file specified - this is not yet supported!" << std::endl;
		}
		if (parameters.size() != filter->Parameters().size())
		{
			std::cout << QString("Invalid number of parameters: %2 expected, %1 were given.")
				.arg(parameters.size())
				.arg(filter->Parameters().size()).toStdString()
				<< std::endl;
			return 1;
		}


		try
		{
			// read input file(s)
			QVector<iAConnector*> cons;
			for (int i = 0; i < inputFiles.size(); ++i)
			{
				if (!quiet)
				{
					std::cout << "Reading input file '" << inputFiles[i].toStdString() << "'" << std::endl;
				}
				iAITKIO::ScalarPixelType pixelType;
				iAITKIO::ImagePointer img = iAITKIO::readFile(inputFiles[i], pixelType, false);
				iAConnector * con = new iAConnector();
				con->SetImage(img);
				cons.push_back(con);
			}

			if (!quiet)
			{
				std::cout << "Running filter '" << filter->Name().toStdString() << "' with parameters: " << std::endl;
				for (int p = 0; p < parameters.size(); ++p)
				{
					std::cout << "    " << filter->Parameters()[p]->Name().toStdString()
						<< "=" << parameters[filter->Parameters()[p]->Name()].toString().toStdString() << std::endl;
				}
			}
			iAProgress progress;
			iACommandLineProgressIndicator progressIndicator(50, quiet);
			QObject::connect(&progress, SIGNAL(pprogress(int)), &progressIndicator, SLOT(Progress(int)));
			filter->SetUp(cons, iAStdOutLogger::Get(), &progress);
			if (!filter->CheckParameters(parameters))
			{   // output already happened in CheckParameters via logger
				return 1;
			}
			filter->Run(parameters);
			// write output file(s)
			if (!quiet)
			{
				std::cout << QString("Writing output to file '%1' (compression: %2)")
					.arg(outputFiles[0]).arg(compress?"on":"off").toStdString()
					<< std::endl;
			}
			iAITKIO::writeFile(outputFiles[0], cons[0]->GetITKImage(), cons[0]->GetITKScalarPixelType(), compress);
			return 0;
		}
		catch (std::exception & e)
		{
			std::cout << "ERROR: " << e.what() << std::endl;
			return 1;
		}
	}
}

int ProcessCommandLine(int argc, char const * const * argv)
{
	auto dispatcher = new iAModuleDispatcher(QFileInfo(argv[0]).absolutePath());
	dispatcher->InitializeModules(iAStdOutLogger::Get());
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
	else
	{
		PrintUsage();
	}
	return 0;
}