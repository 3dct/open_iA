// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include "iAAttributes.h"

#include <QString>
#include <QObject>

#include <memory>

class iADataSet;
class iADataSetRenderer;
class iAMdiChild;
class iAOutlineImpl;
class iAPreferences;
class iAProgress;

class vtkPlane;
class vtkProp3D;
class vtkRenderer;

class QAction;

//! Base class for handling the viewing of an iADataSet in the GUI.
//! Holds all additional data structures (GUI, computed values, etc.) necessary to display it,
//! in addition to the dataset itself (e.g., the histogram for a volume dataset)
class iAguibase_API iADataSetViewer: public QObject
{
	Q_OBJECT
public:

	//! key that specifies which views or renderers the viewer currently has visible
	static const QString RenderFlags;

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
	//! Retrieves the list of all attributes for this viewer, merged with their current values as default values:
	iAAttributes attributesWithValues() const;
	//! Retrieve only the current attribute values
	QVariantMap attributeValues() const;

	//! @{
	//! Retrieves the 3D renderer for this dataset (if any; by default, no renderer)
	iADataSetRenderer* renderer();
	iADataSetRenderer const * renderer() const;
	//! @}

	//! Call to change the attributes of this viewer
	void setAttributes(QVariantMap const& values);

	//! called before a dataset is stored; stores the current viewer state into the metadata of the linked dataset (via attributeValues)
	void storeState();

	//!
	void setRenderFlag(QChar const& flag, bool enable);
	bool renderFlagSet(QChar const& flag) const;

	virtual void setPickable(bool pickable);
	void setPickActionVisible(bool visible);

	//! TODO NEWIO: improve!
	virtual void slicerRegionSelected(double minVal, double maxVal, uint channelID);
	virtual uint slicerChannelID();

signals:
	void dataSetChanged(size_t dataSetIdx);
	void removeDataSet(size_t dataSetIdx);

protected:
	iADataSetViewer(iADataSet * dataSet);
	virtual ~iADataSetViewer();
	//! adds an attribute that can be modified by the user to change the appearance of some aspect of the viewer
	void addAttribute(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());
	//! helper function for creating and adding a toggling action for switching some aspect of this viewer
	//! needs to be called before calling createGUI of this base class (which passes the view actions created so far
	//! to the dataset list. Actions are shown in reverse order to call to this function, since we want to show
	//! the ones common to all viewers first, but these are only created in createGUI of this base class
	QAction* addViewAction(QString const& name, QString const& iconName, bool checked, std::function<void(bool)> handler);

	QVariantMap m_attribValues;
	iADataSet* m_dataSet;    //!< the dataset for which this viewer is responsible

private:

	//! return any optional additional state required when storing the viewer's state
	virtual QVariantMap additionalState() const;

	iAAttributes m_attributes;     //!< attributes of this viewer that can be changed by the user
	std::shared_ptr<iADataSetRenderer> m_renderer; //!< the 3D renderer for this dataset (optional).
	std::shared_ptr<iADataSetRenderer> m_magicLensRenderer; //!< the 3D renderer for this dataset (optional).
	QAction* m_pickAction;
	QVector<QAction*> m_viewActions;
	QVector<QAction*> m_editActions;

	//! Called when the attributes have changed; override in derived classes to apply such a change to renderer (default implementation is empty)
	virtual void applyAttributes(QVariantMap const& values);
	//! Called to create a 3D renderer for the dataset. Override in derived class; used for both the "normal" 3D renderer and the magic lens renderer.
	virtual std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren);
};

class iAguibase_API iAMeshViewer : public iADataSetViewer
{
public:
	iAMeshViewer(iADataSet * dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
};

class iAguibase_API iAGraphViewer : public iADataSetViewer
{
public:
	iAGraphViewer(iADataSet * dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
};

class iAguibase_API iAGeometricObjectViewer : public iADataSetViewer
{
public:
	iAGeometricObjectViewer(iADataSet * dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
};

//! A "viewer" for project files; special insofar as it overrides createGUI and
//! doesn't call the one from the base class; meaning the project file dataset
//! won't get added to the dataset list, the viewer only cares about loading all
//! datasets in the project
class iAguibase_API iAProjectViewer : public iADataSetViewer
{
public:
	iAProjectViewer(iADataSet * dataSet);
	void createGUI(iAMdiChild* child, size_t dataSetIdx) override;
private:
	//! IDs of the loaded datasets, to check against the list of datasets rendered
	std::vector<size_t> m_loadedDataSets;
	//! number of datasets to load in total
	size_t m_numOfDataSets;
};

iAguibase_API std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet * dataSet);
