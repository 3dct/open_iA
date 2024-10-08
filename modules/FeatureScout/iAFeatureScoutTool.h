// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "featurescout_export.h"

#include <iACsvConfig.h>

#include <iATool.h>
#include <iAVec3.h>

#include <QObject>

#include <map>
#include <memory>
#include <vector>

class dlg_FeatureScout;
class iAObjectsData;
class iAObjectVis;

class QSettings;

//! Main entry point class for Feature Scout.
//! Three ways exist of starting feature scout:
//!     - the two addToChild method variants:
//!         - one expecting just a csv file name, it guesses the format from it
//!         - one expecting a full csv config object
//!     - creating an object, e.g. via the create method, and then calling loadState
class FeatureScout_API iAFeatureScoutTool : public QObject, public iATool
{
	Q_OBJECT
public:
	static const QString ID;
	//! add FeatureScout to the given child window, guessing config parameters from the given csv file naem
	static bool addToChild(iAMdiChild* child, const QString& csvFileName);
	//! add FeatureScout to the given child window, using the given configuration to set up
	static bool addToChild(iAMdiChild* child, iACsvConfig const& csvConfig);
	//! add FeatureScout to the given child window, using the given data and config to set up
	static bool addToChild(iAMdiChild* child, iACsvConfig const& csvConfig, std::shared_ptr<iAObjectsData> objData);

	iAFeatureScoutTool(iAMainWindow* mainWnd, iAMdiChild* child);

	//! to ensure correct "order" of deletion (that for example object vis registered with renderer
	//! can de-register itself, before renderer gets destroyed - if destroyed through MdiChild's
	//! destructing its child widgets, then this happens after renderer is destroyed!
	virtual ~iAFeatureScoutTool();

	void saveState(QSettings & state, QString const& fileName) override;
	void loadState(QSettings & state, QString const& fileName) override;

	void setOptions(iACsvConfig const& config);

	static iAObjectType guessFeatureType(QString const& csvFileName);

	//! the index of the dataset used for visualizations
	size_t visDataSetIdx() const;

	//! set a selection from the outside
	void setSelection(std::vector<size_t> const & selection);
	//! retrieve the current selected objects
	std::vector<size_t> selection();

	//! retrieve access to main featurescout class
	dlg_FeatureScout* featureScout();

signals:
	//! fires whenever the selection of the associated 3D object visualization is changed
	void selectionChanged();

private:
	bool initFromConfig(iAMdiChild* child, iACsvConfig const& csvConfig);
	bool init(iAMdiChild* child, iACsvConfig const& csvConfig, std::shared_ptr<iAObjectsData> objData);
	iACsvConfig m_config;
	dlg_FeatureScout * m_featureScout;
	//! @{ for the case of labelled volume data (for which the viewer cannot be created automatically), we need to store data ourselves
	std::shared_ptr<iAObjectsData> m_objData;
	std::shared_ptr<iAObjectVis> m_objVis;
	//! @}
	size_t m_objDataSetIdx;
};
