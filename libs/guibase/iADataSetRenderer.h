// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

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
	static constexpr const char Position[] = "Position";
	static constexpr const char Orientation[] = "Orientation";
	static constexpr const char OutlineColor[] = "Box Color";
	static constexpr const char Pickable[] = "Pickable";
	static constexpr const char Shading[] = "Shading";
	static constexpr const char AmbientLighting[] = "Ambient lighting";
	static constexpr const char DiffuseLighting[] = "Diffuse lighting";
	static constexpr const char SpecularLighting[] = "Specular lighting";
	static constexpr const char SpecularPower[] = "Specular power";

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

	//! default attributes of the dataset renderer
	static iAAttributes& defaultAttributes();

protected:
	//void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
	//	double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

	//! needs to be called by derived classes whenever the bounds of the dataset change (position, orientation, ...)
	void updateOutlineTransform();

	//! attributes of the specific renderer; this is typically the default attributes plus dataset-specific attributes
	virtual iAAttributes const & attributes() const;

	//! convenience methods for derived classes: to be used at end of constructor to initialize attribute values to default,
	//! with the option to override these default values (e.g. by ones loaded along with the dataset in a project file)
	//! @param defaultAttr the list of default attributes to set
	//! @param overrideValues the values overriding the defaults
	void setDefaultAttributes(iAAttributes const& defaultAttr, QVariantMap const& overrideValues);

	//! The VTK renderer used for showing this dataset.
	//! Note that this pointer may be set to nullptr if the renderer is deleted before;
	//! so in derived classes, one should always check if it is set before accessing it!
	vtkRenderer* m_renderer;
	mutable QVariantMap m_attribValues;

private:
	//! @{ internal logic of showing/hiding dataset in renderer; called internally from setVisible; implement in derived classes
	virtual void showDataSet() = 0;
	virtual void hideDataSet() = 0;
	//! @}
	//! called when the attributes have changed; derive to apply such a change to renderer
	virtual void applyAttributes(QVariantMap const& values) = 0;

	void clearRenderer();

	std::shared_ptr<iAOutlineImpl> m_outline;
	bool m_visible;
	unsigned long m_renderObserverTag;
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
