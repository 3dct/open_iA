#pragma once

#include <iAguibase_export.h>

#include <memory>

class iADataSet;

class iARenderer;

//! abstract interface for a class that renders a dataset (in a vtkRenderer)
class iADataSetRenderer
{
public:
	iADataSetRenderer(iARenderer* renderer);
	virtual void show() = 0;
	virtual void hide() = 0;
protected:
	iARenderer* m_renderer;
};

//! Factory function to create a renderer for a given dataset
iAguibase_API std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer);
