/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "io/csv_config.h"
#include "io/iACsvIO.h"

#include "iAModuleInterface.h"
#include "iAObjectAnalysisType.h"
#include "mdichild.h"

class QDockWidget;
class dlg_FeatureScout;
class iAFeatureScoutToolbar;



class iAFeatureScoutModuleInterface : public iAModuleInterface
{
	Q_OBJECT

public:
	void Initialize();
	void hideFeatureScoutToolbar();
private slots:

	void FeatureScoutWithCSV();

	void FeatureScout();
	void FeatureScout_Options();
	void onChildClose();
private:
	virtual iAModuleAttachmentToChild * CreateAttachment(MainWindow* mainWnd, iAChildData childData);
	bool filter_FeatureScout(MdiChild* mdiChild, QString fileName, iAObjectAnalysisType filterID, csvConfig::configPararams *FileParams, const bool is_csvOnly, const QSharedPointer<QStringList> &selHeader);
	void SetupToolbar();
	void setFeatureScoutRenderSettings();
	void initializeFeatureScoutStartUp(QString &item, QStringList &items, QString &fileName, QMap<QString,
		iAObjectAnalysisType> &objectMap, QString &filterName, const bool isCsvOnly, csvConfig::configPararams *FileParams, const QSharedPointer<QStringList> &selHeaders);
	iAFeatureScoutToolbar * tlbFeatureScout;

	iACsvIO io;

};
