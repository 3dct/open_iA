#pragma once

#include <iAguibase_export.h>

#include "iAAttributes.h"

#include <memory>

class iADataSet;

class iARenderer;

//! abstract interface for a class for 3D rendering of a dataset (in an iARenderer)
class iAguibase_API iADataSetRenderer
{
public:
	// necessary to implement:
	iADataSetRenderer(iARenderer* renderer);
	void show();
	void hide();
	bool isVisible() const;

	iAAttributes const & attributes() const;
	virtual void setAttributes(QMap<QString, QVariant> values);

	// optional additional features:
	/*
	virtual iAAABB bounds(); // -> separate show bounds
	
	// interactions:
	virtual void setMovable(bool movable);

	void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	void removeCuttingPlanes();

	// retrieve/set orientation/position:
	virtual double const* orientation() const;
	virtual double const* position() const;
	virtual void setPosition(double*);
	virtual void setOrientation(double*);
	*/

protected:
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());
	iARenderer* m_renderer;

private:
	//! @{ internal logic of showing/hiding dataset in renderer; never called directly, see show/hide above
	virtual void showDataSet() = 0;
	virtual void hideDataSet() = 0;
	//! @}

	iAAttributes m_attributes;
	bool m_visible;
};

//! Factory function to create a renderer for a given dataset
iAguibase_API std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer);
