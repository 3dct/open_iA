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
#include <QObject>

#include <memory>

class iAAABB;
class iADataSet;
class iADataSetRenderer;
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
class iAguibase_API iADataSetViewer: public QObject
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
	//! Should contain all things that need to be done in the GUI thread for viewing this dataset
	//! The default implementation creates a 3D renderer (via createRenderer method) and adds an entry
	//! to the dataset list. If you want these things to happen, but additionally some other things,
	//! call this function from the derived method (i.e. iADataSetViewer::createGUI).
	//! Sometimes, you might not want these things to happen (as e.g. the iAProjectViewer does, since it does not need a 3D 
	//! viewer nor a dataset list entry), 
	virtual void createGUI(iAMdiChild* child, size_t dataSetIdx);
	//! Get information to display about the dataset
	virtual QString information() const;
	//! Called after dataset properties have changed, for potential required GUI updates
	virtual void dataSetChanged();
	//! Retrieves the list of attributes, merged with their current values as default values:
	iAAttributes attributesWithValues() const;

	//! @{
	//! Retrieves the 3D renderer for this dataset (if any; by default, no renderer)
	iADataSetRenderer* renderer();
	iADataSetRenderer const * renderer() const;
	//! @}

	//! Call to change the attributes of this viewer
	void setAttributes(QVariantMap const& values);

	virtual void setPickable(bool pickable);

	//! TODO NEWIO: improve!
	virtual void slicerRegionSelected(double minVal, double maxVal, uint channelID);
	virtual uint slicerChannelID();

protected:
	iADataSetViewer(iADataSet const * dataSet);
	virtual ~iADataSetViewer();
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());
	virtual std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren);
	virtual bool hasSlicerVis() const;
	virtual void setSlicerVisibility(bool visible);

	QVariantMap m_attribValues;
	iADataSet const* m_dataSet;    //!< the dataset for which this viewer is responsible
private:
	iAAttributes m_attributes;     //!< attributes of this viewer that can be changed by the user
	std::shared_ptr<iADataSetRenderer> m_renderer; //!< the 3D renderer for this dataset (optional).
	std::shared_ptr<iADataSetRenderer> m_magicLensRenderer; //!< the 3D renderer for this dataset (optional).

	//! called when the attributes have changed; derive to apply such a change to renderer
	virtual void applyAttributes(QVariantMap const& values);
};

class iAguibase_API iAMeshViewer : public iADataSetViewer
{
public:
	iAMeshViewer(iADataSet const * dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
};

class iAguibase_API iAGraphViewer : public iADataSetViewer
{
public:
	iAGraphViewer(iADataSet const * dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
};

class iAguibase_API iAGeometricObjectViewer : public iADataSetViewer
{
public:
	iAGeometricObjectViewer(iADataSet const* dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
};

class iAguibase_API iAProjectViewer : public iADataSetViewer
{
public:
	iAProjectViewer(iADataSet const * dataSet);
	void createGUI(iAMdiChild* child, size_t dataSetIdx) override;
private:
	//! IDs of the loaded datasets, to check against the list of datasets rendered
	std::vector<size_t> m_loadedDataSets;
	//! number of datasets to load in total
	size_t m_numOfDataSets;
};

iAguibase_API std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet const* dataSet);
