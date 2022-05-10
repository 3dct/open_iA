#pragma once

#include <iAguibase_export.h>

#include "iAAttributes.h"

#include <memory>

class iADataSet;

class iARenderer;

//! abstract interface for a class that renders a dataset (in a vtkRenderer)
class iAguibase_API iADataSetRenderer
{
public:
	iADataSetRenderer(iARenderer* renderer);
	virtual void show() = 0;
	virtual void hide() = 0;
	iAAttributes const & attributes() const;
	virtual void setAttributes(QMap<QString, QVariant> values);

protected:
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());
	iARenderer* m_renderer;
	iAAttributes m_attributes;
};

//! Factory function to create a renderer for a given dataset
iAguibase_API std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer);
