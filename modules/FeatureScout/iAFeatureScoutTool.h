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
#include "FeatureScout_export.h"

#include <iACsvConfig.h>

#include <iATool.h>
#include <iAVec3.h>

#include <vtkSmartPointer.h>

#include <QObject>
#include <QSharedPointer>

#include <map>
#include <vector>

class dlg_FeatureScout;

class vtkTable;

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
	static std::shared_ptr<iATool> create(iAMainWindow* mainWnd, iAMdiChild* child);
	//! add FeatureScout to the given child window, guessing config parameters from the given csv file naem
	static bool addToChild(iAMdiChild* child, const QString& csvFileName);
	//! add FeatureScout to the given child window, using the given configuration to set up
	static bool addToChild(iAMdiChild* child, iACsvConfig const& csvConfig);

	iAFeatureScoutTool(iAMainWindow* mainWnd, iAMdiChild* child);
	virtual ~iAFeatureScoutTool();

	//! to ensure correct "order" of deletion (that for example object vis registered with renderer
	//! can de-register itself, before renderer gets destroyed - if destroyed through MdiChild's
	//! destructing its child widgets, then this happens after renderer is destroyed!
	void saveState(QSettings& projectFile, QString const& fileName) override;
	void loadState(QSettings& projectFile, QString const& fileName) override;

	void setOptions(iACsvConfig const& config);

	static iAObjectType guessFeatureType(QString const& csvFileName);

private:
	bool initFromConfig(iAMdiChild* child, iACsvConfig const& csvConfig);
	void init(int filterID, QString const& fileName, vtkSmartPointer<vtkTable> csvtbl, int visType,
		QSharedPointer<QMap<uint, uint>> columnMapping, std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo,
		int cylinderQuality, size_t segmentSkip);
	iACsvConfig m_config;
	dlg_FeatureScout * m_featureScout;
};
