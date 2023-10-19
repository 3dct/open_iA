// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADataSetViewer.h"

#include "iaguibase_export.h"

//! Dataset viewer for surface mesh data.
class iAguibase_API iAMeshViewer : public iADataSetViewer
{
public:
	iAMeshViewer(iADataSet* dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren, QVariantMap const& paramValues) override;
};

//! Dataset viewer for graph data.
class iAguibase_API iAGraphViewer : public iADataSetViewer
{
public:
	iAGraphViewer(iADataSet* dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren, QVariantMap const& paramValues) override;
};

//! Dataset viewer for simple geometric objects.
class iAguibase_API iAGeometricObjectViewer : public iADataSetViewer
{
public:
	iAGeometricObjectViewer(iADataSet* dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren, QVariantMap const& paramValues) override;
	void applyAttributes(QVariantMap const& values) override;
};

//! A "viewer" for project files.
//! Special insofar as it overrides createGUI and
//! doesn't call the one from the base class; meaning the project file dataset
//! won't get added to the dataset list, the viewer only cares about loading all
//! datasets in the project
class iAguibase_API iAProjectViewer : public iADataSetViewer
{
public:
	iAProjectViewer(iADataSet* dataSet);
	void createGUI(iAMdiChild* child, size_t dataSetIdx) override;
private:
	//! IDs of the loaded datasets, to check against the list of datasets rendered
	std::vector<size_t> m_loadedDataSets;
	std::set<size_t> m_renderedDataSets;
	//! number of datasets to load in total
	size_t m_numOfDataSets;
};

#include <iADataSetType.h>

using iADataSetViewerCreateFuncPtr = std::shared_ptr<iADataSetViewer>(*)(iADataSet*);

iAguibase_API std::map<iADataSetType, iADataSetViewerCreateFuncPtr>& dataSetViewerFactoryMap();

iAguibase_API std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet* dataSet);
