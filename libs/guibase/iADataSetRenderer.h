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
	
	//! The list of display attributes of this renderer
	iAAttributes const & attributes() const;
	//! Call to change the attributes of this renderer
	void setAttributes(QMap<QString, QVariant> const& values);
	//! The current values of the display attributes
	QMap<QString, QVariant> const & attributeValues() const;

	//! Retrieves the list of attributes, merged with their current values as default values:
	iAAttributes attributesWithValues() const;

	//! Set the visibility of the axis-aligned bounding box:
	void setBoundsVisible(bool visible);
	//! The coordinates of the axis-aligned bounding box (of the dataset, untransformed to any current position/orientation changes)
	virtual iAAABB bounds() = 0;

	// optional additional features:
	/*
	
	// interactions:
	virtual void setMovable(bool movable);

	void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	void removeCuttingPlanes();
	*/

	// retrieve/set orientation/position:
	virtual double const* orientation() const =0;
	virtual double const* position() const = 0;
	//virtual void setPosition(double*) = 0;
	//virtual void setOrientation(double*) = 0;

protected:
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());
	iARenderer* m_renderer;

private:
	//! @{ internal logic of showing/hiding dataset in renderer; called internally from setVisible; implement in derived classes
	virtual void showDataSet() = 0;
	virtual void hideDataSet() = 0;
	//! @}
	//! called when the attributes have changed; derive to apply such a change to renderer
	virtual void applyAttributes() = 0;

	iAAttributes m_attributes;
	QMap<QString, QVariant> m_attribValues;
	std::shared_ptr<iAOutlineImpl> m_outline;
	bool m_visible;
};

//! Factory function to create a renderer for a given dataset
iAguibase_API std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer);
