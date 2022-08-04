#pragma once

#include "iAgui_export.h"

#include <memory>

class iADataSet;
class iAMdiChild;

//! Abstract base class for data linked to a dataset required for displaying it,
//! in addition to the dataset itself (e.g., the histogram for a volume dataset)
//! TODO: Find a better name!
class iAgui_API iADataForDisplay
{
public:
	//! called from GUI thread when the data computation (via createDataForDisplay method below) is complete
	virtual void show(iAMdiChild* child) =0;
	//! called when the dataset is removed and its related controls should close down
	virtual void close() = 0;
};

std::shared_ptr<iADataForDisplay> createDataForDisplay(iADataSet* dataSet);
