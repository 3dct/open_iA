#include "iACommandLineProcessor.h"

#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAModuleDispatcher.h"
#include "iAStringHelper.h"

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
	if (argc > 1 && QString(argv[1]) == "-l")
	{
#ifdef _MSC_VER
		if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
		{
			AllocConsole();
		}
		FILE * file;
		freopen_s(&file, "CONOUT$", "w", stdout);
#endif
		auto dispatcher = new iAModuleDispatcher(QFileInfo(argv[0]).absolutePath());
		dispatcher->InitializeModules(&iAStdOutLogger::Get());
		auto filterFactories = iAFilterRegistry::FilterFactories();
		std::cout << "Available filters:" << std::endl;
		for (auto factory: filterFactories)
		{
			auto filter = factory->Create();
			std::cout << filter->Name().toStdString() << std::endl
				<< "        " << AbbreviateDesc(filter->Description()).toStdString() << std::endl;
		}
		return true;
	}
	return false;
}