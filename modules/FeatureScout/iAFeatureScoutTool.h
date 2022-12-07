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

#include <iATool.h>
#include <iAVec3.h>

#include <iACsvConfig.h>

#include <vtkSmartPointer.h>

#include <QObject>
#include <QSharedPointer>

#include <map>
#include <vector>

class dlg_FeatureScout;

class vtkTable;

class QSettings;

class iAFeatureScoutTool : public QObject, public iATool
{
	Q_OBJECT
public:
	static const QString ID;
	static std::shared_ptr<iATool> create(iAMainWindow* mainWnd, iAMdiChild* child)
	{
		return std::make_shared<iAFeatureScoutTool>(mainWnd, child);
	}
	iAFeatureScoutTool(iAMainWindow* mainWnd, iAMdiChild* child);
	virtual ~iAFeatureScoutTool();
	void init(int filterID, QString const & fileName, vtkSmartPointer<vtkTable> csvtbl, int visType,
		QSharedPointer<QMap<uint, uint> > columnMapping, std::map<size_t,
		std::vector<iAVec3f> > & curvedFiberInfo, int cylinderQuality, size_t segmentSkip);
	//! to ensure correct "order" of deletion (that for example object vis registered with renderer
	//! can de-register itself, before renderer gets destroyed - if destroyed through MdiChild's
	//! destructing its child widgets, then this happens after renderer is destroyed!
	void saveState(QSettings& projectFile, QString const& fileName) override;
	void loadState(QSettings& projectFile, QString const& fileName) override;

	void setOptions(iACsvConfig const& config);
private:
	iACsvConfig m_config;
	dlg_FeatureScout * m_featureScout;
};
