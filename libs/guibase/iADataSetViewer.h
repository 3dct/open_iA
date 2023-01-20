/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAguibase_export.h"

#include "iAAttributes.h"

#include <QString>

#include <memory>

class iAAABB;
class iADataSet;
class iAMdiChild;
class iAOutlineImpl;
class iAPreferences;
class iAProgress;

class vtkPlane;
class vtkProp3D;
class vtkRenderer;

//! Base class for handling the viewing of an iADataSet in the GUI.
//! Holds all additional data structures (GUI, computed values, etc.) necessary to display it,
//! in addition to the dataset itself (e.g., the histogram for a volume dataset)
class iAguibase_API iADataSetViewer
{
public:
	//! @{
	//! general 3D rendering properties for every kind of dataset
	static const QString Position;
	static const QString Orientation;
	static const QString OutlineColor;
	static const QString Pickable;
	static const QString Shading;
	static const QString AmbientLighting;
	static const QString DiffuseLighting;
	static const QString SpecularLighting;
	static const QString SpecularPower;
	//! @}

	//! called directly after the dataset is loaded, should do anything that needs to be computed in the background
	virtual void prepare(iAPreferences const & pref, iAProgress* p);
	//! all things that need to be done in the GUI thread for viewing the dataset
	virtual void createGUI(iAMdiChild* child) = 0;
	//! Get information to display about the dataset
	virtual QString information() const;
	//! Called after dataset properties have changed, for potential required GUI updates
	virtual void dataSetChanged();
	//! Retrieves the list of attributes, merged with their current values as default values:
	iAAttributes attributesWithValues() const;

	//! Set visibility of dataset in 3D renderer
	void setVisible(bool visible);
	//! Whether dataset is currently visible in 3D renderer
	bool isVisible() const;
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
	virtual double const* position() const = 0;
	//! Get Orientation of this dataset in scene
	virtual double const* orientation() const = 0;
	//! Set position of this dataset in scene
	virtual void setPosition(double pos[3]) = 0;
	//! Set orientation of this dataset in scene
	virtual void setOrientation(double ori[3]) = 0;

	virtual vtkProp3D* vtkProp() = 0;

	virtual void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	virtual void removeCuttingPlanes();

protected:
	iADataSetViewer(iADataSet const * dataSet);
	virtual ~iADataSetViewer();
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

	//! needs to be called by derived classes whenever the bounds of the dataset change (position, orientation, ...)
	void updateOutlineTransform();

	vtkRenderer* m_renderer;
	QVariantMap m_attribValues;
	iADataSet const* m_dataSet;    //!< reference to the dataset for which this viewer is responsible
private:
	iAAttributes m_attributes;     //!< 
	std::shared_ptr<iAOutlineImpl> m_outline;
	bool m_visible;

	//! @{ internal logic of showing/hiding dataset in renderer; called internally from setVisible; implement in derived classes
	virtual void showDataSet() = 0;
	virtual void hideDataSet() = 0;
	//! @}
	//! called when the attributes have changed; derive to apply such a change to renderer
	virtual void applyAttributes(QVariantMap const& values) = 0;
};

class iAguibase_API iAMeshViewer : public iADataSetViewer
{
public:
	iAMeshViewer(iADataSet const * dataSet);
	void prepare(iAPreferences const& pref, iAProgress* p) override;
	void createGUI(iAMdiChild* child) override;
};

#include <QObject>

class iAguibase_API iAProjectViewer : public QObject, public iADataSetViewer
{
public:
	iAProjectViewer(iADataSet const * dataSet);
	void createGUI(iAMdiChild* child) override;
private:
	//! IDs of the loaded datasets, to check against the list of datasets rendered
	std::vector<size_t> m_loadedDataSets;
	//! number of datasets to load in total
	size_t m_numOfDataSets;
};

iAguibase_API std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet const* dataSet);