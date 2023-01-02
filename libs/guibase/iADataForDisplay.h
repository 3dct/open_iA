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

#include <QString>

#include <memory>

class iADataSet;
class iAMdiChild;
class iAPreferences;
class iAProgress;

//! Base class for data linked to a dataset required for displaying it,
//! in addition to the dataset itself (e.g., the histogram for a volume dataset)
//! TODO: Find a better name!
class iAguibase_API iADataForDisplay
{
public:
	iADataForDisplay(iADataSet* dataSet);
	//! called from GUI thread when the data computation is complete
	virtual void show(iAMdiChild* child);
	//! called when the dataset is removed and its related controls should close down
	virtual ~iADataForDisplay();
	//! Get information to display about the dataset
	virtual QString information() const;
	//! Called when preferences have changed
	virtual void applyPreferences(iAPreferences const& prefs);
	//! Called after preferences have been applied, for potential required GUI updates
	virtual void updatedPreferences();
	//! Called after dataset properties have changed, for potential required GUI updates
	virtual void dataSetChanged();
protected:
	iADataSet* dataSet();
private:
	iADataSet* m_dataSet;
};

//! base class for handling the viewing of an iADataSet in the GUI
class iAguibase_API iADataSetViewer
{
public:
	//! called directly after the dataset is loaded, should do anything that needs to be computed in the background
	virtual void prepare(iAPreferences const & pref);
	//! all things that need to be done in the GUI thread for viewing the dataset
	virtual void createGUI(iAMdiChild* child) = 0;
	// //! indicate whether it is necessary to keep this class after 
	// bool keepAround() const;
protected:
	iADataSetViewer(iADataSet const * dataSet);
	virtual ~iADataSetViewer();
	iADataSet const* m_dataSet;
};

class iAguibase_API iAVolumeViewer: public iADataSetViewer
{
public:
	iAVolumeViewer(iADataSet const * dataSet);
	void prepare(iAPreferences const& pref) override;
	void createGUI(iAMdiChild* child) override;
};

class iAguibase_API iAMeshViewer : public iADataSetViewer
{
public:
	iAMeshViewer(iADataSet const * dataSet);
	void prepare(iAPreferences const& pref) override;
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