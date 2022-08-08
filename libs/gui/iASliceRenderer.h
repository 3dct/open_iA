#pragma once

#include <iAgui_export.h>

#include <array>
#include <memory>

class iADataSet;
class iADataForDisplay;
class iAMdiChild;
class iASlicerImpl;

class iAgui_API iASliceRenderer
{
public:
	virtual void setVisible(bool visible);
};

iAgui_API std::shared_ptr<iASliceRenderer> createSliceRenderer(iADataSet* dataset, iADataForDisplay* dataForDisplay,
	std::array<iASlicerImpl*, 3> slicer, iAMdiChild* child);
