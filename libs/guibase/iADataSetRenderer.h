#pragma once

#include <iAguibase_export.h>

#include "iAAttributes.h"

#include <memory>

class iAAABB;
class iADataSet;
class iAOutlineImpl;

class iARenderer;

//! abstract interface for a class for 3D rendering of a dataset (in an iARenderer)
class iAguibase_API iADataSetRenderer
{
public:
	//! Create a dataset renderer
	iADataSetRenderer(iARenderer* renderer);
	//! Set visibility of dataset
	void setVisible(bool visible);
	//! Whether dataset is currently visible
	bool isVisible() const;
	
	//! Retrieves the list of attributes, merged with their current values as default values:
	iAAttributes attributesWithValues() const;
	//! Call to change the attributes of this renderer
	void setAttributes(QMap<QString, QVariant> const& values);

	//! Set the visibility of the axis-aligned bounding box:
	void setBoundsVisible(bool visible);
	//! The coordinates of the axis-aligned bounding box (of the dataset, untransformed to any current position/orientation changes)
	virtual iAAABB bounds() = 0;

	//! Get Position of this dataset in scene:
	virtual double const* position() const = 0;
	//! Get Orientation of this dataset in scene:
	virtual double const* orientation() const =0;

	// optional additional features:
	/*
	
	// interactions:
	virtual void setMovable(bool movable);

	void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	void removeCuttingPlanes();
	*/

protected:
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

	//! needs to be called by derived classes whenever the bounds of the dataset change (position, orientation, ...)
	void updateOutlineTransform();

	iARenderer* m_renderer;
	QMap<QString, QVariant> m_attribValues;

private:
	//! @{ internal logic of showing/hiding dataset in renderer; called internally from setVisible; implement in derived classes
	virtual void showDataSet() = 0;
	virtual void hideDataSet() = 0;
	//! @}
	//! called when the attributes have changed; derive to apply such a change to renderer
	virtual void applyAttributes(QMap<QString, QVariant> const& values) = 0;

	iAAttributes m_attributes;
	std::shared_ptr<iAOutlineImpl> m_outline;
	bool m_visible;
};

//! Factory function to create a renderer for a given dataset
iAguibase_API std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer);
