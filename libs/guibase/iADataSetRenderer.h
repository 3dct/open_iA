// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAguibase_export.h>

#include "iAAttributes.h"

#include <memory>

class iAAABB;
class iADataSet;
class iADataForDisplay;
class iAOutlineImpl;

class vtkPlane;
class vtkProp3D;
class vtkRenderer;

//! Abstract interface for 3D renderers of a dataset (in an iARenderer).
class iAguibase_API iADataSetRenderer
{
public:
	static const QString Position;
	static const QString Orientation;
	static const QString OutlineColor;
	static const QString Pickable;
	static const QString Shading;
	static const QString AmbientLighting;
	static const QString DiffuseLighting;
	static const QString SpecularLighting;
	static const QString SpecularPower;

	//! Create a dataset renderer
	iADataSetRenderer(vtkRenderer* renderer);
	//! called when dataset renderer is removed from display and destroyed.
	//! Only takes care of removing outline from display; derived classes need to remove their props from renderers themselves!
	virtual ~iADataSetRenderer();
	//! Set visibility of dataset
	void setVisible(bool visible);
	//! Whether dataset is currently visible
	bool isVisible() const;

	//! Retrieves the list of attributes, merged with their current values as default values:
	iAAttributes attributesWithValues() const;
	//! Retrieve only the current attribute values
	virtual QVariantMap attributeValues() const;
	//! Call to change the attributes of this renderer
	void setAttributes(QVariantMap const& values);

	//! convenience method for setting the pickable attribute
	void setPickable(bool pickable);
	//! whether dataset is currently set to be pickable
	bool isPickable() const;

	//! Set the visibility of the axis-aligned bounding box:
	void setBoundsVisible(bool visible);
	//! The coordinates of the axis-aligned bounding box (of the dataset, untransformed to any current position/orientation changes)
	virtual iAAABB bounds() = 0;

	//! Get Position of this dataset in scene
	//! TODO: make deliver std::array by value to avoid pointer to internal data!
	virtual double const* position() const = 0;
	//! Get Orientation of this dataset in scene
	//! TODO: make deliver std::array by value to avoid pointer to internal data!
	virtual double const* orientation() const = 0;
	//! Set position of this dataset in scene
	virtual void setPosition(double pos[3]) = 0;
	//! Set orientation of this dataset in scene
	virtual void setOrientation(double ori[3]) = 0;

	virtual vtkProp3D* vtkProp() = 0;

	//! optional additional control widget needed for additional settings in the renderer
	//virtual QWidget* controlWidget();

	// optional additional features:
	/*

	// interactions:
	virtual void setMovable(bool movable);
	*/

	virtual void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	virtual void removeCuttingPlanes();

protected:
	//void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
	//	double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

	//! needs to be called by derived classes whenever the bounds of the dataset change (position, orientation, ...)
	void updateOutlineTransform();

	virtual iAAttributes const & attributes() const;
	static iAAttributes& defaultAttributes();

	vtkRenderer* m_renderer;
	mutable QVariantMap m_attribValues;

private:
	//! @{ internal logic of showing/hiding dataset in renderer; called internally from setVisible; implement in derived classes
	virtual void showDataSet() = 0;
	virtual void hideDataSet() = 0;
	//! @}
	//! called when the attributes have changed; derive to apply such a change to renderer
	virtual void applyAttributes(QVariantMap const& values) = 0;

	std::shared_ptr<iAOutlineImpl> m_outline;
	bool m_visible;
};

template <class T>
void applyLightingProperties(T* prop, QVariantMap const& values, QString const & prefix = "")
{
	prop->SetAmbient(values[prefix + iADataSetRenderer::AmbientLighting].toDouble());
	prop->SetDiffuse(values[prefix + iADataSetRenderer::DiffuseLighting].toDouble());
	prop->SetSpecular(values[prefix + iADataSetRenderer::SpecularLighting].toDouble());
	prop->SetSpecularPower(values[prefix + iADataSetRenderer::SpecularPower].toDouble());
	// prop->SetShading(values[prefix + iADataSetRenderer::Shading].toBool()); // not generic - Shading/Shade
}
