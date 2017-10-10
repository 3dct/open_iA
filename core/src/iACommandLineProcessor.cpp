#include "iACommandLineProcessor.h"

#include "iAAttributeDescriptor.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAModuleDispatcher.h"
#include "iAStringHelper.h"
#include "iAValueType.h"

#include "QFileInfo"

#include <iostream>

namespace
{
	QString AbbreviateDesc(QString desc)
	{
		int brpos = desc.indexOf("<br/>");
		return brpos != -1 ? desc.left(brpos) : desc;
	}
}

int ProcessCommandLine(int argc, char const * const * argv)
{
	auto dispatcher = new iAModuleDispatcher(QFileInfo(argv[0]).absolutePath());
	dispatcher->InitializeModules(&iAStdOutLogger::Get());
	auto filterFactories = iAFilterRegistry::FilterFactories();
	if (argc > 1 && QString(argv[1]) == "-l")
	{
		std::cout << "Available filters:" << std::endl;
		for (auto factory: filterFactories)
		{
			auto filter = factory->Create();
			std::cout << filter->Name().toStdString() << std::endl
				<< "        " << AbbreviateDesc(filter->Description()).toStdString() << std::endl;
		}
		return true;
	}
	else if (argc > 2 && QString(argv[1]) == "-h")
	{
		for (auto factory : filterFactories)
		{
			auto filter = factory->Create();
			if (filter->Name() == argv[2])
			{
				std::cout << filter->Name().toStdString() << ":" << std::endl
					<< filter->Description().replace("<br/>", "\n").toStdString() << std::endl;
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
		}
	}
	else
	{
		std::cout << "open_iA command line tool. Usage:" << std::endl
			<< "    open_iA_cmd.exe [-l] [-h FilterName] [-r FilterName Parameters ...]" << std::endl
			<< "         -l             List available filters" << std::endl
			<< "         -h FilterName  Print help on a specific filter" << std::endl;
	}
	return false;
}