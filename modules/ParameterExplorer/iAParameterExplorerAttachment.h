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
#include <iAModuleAttachmentToChild.h>

#include <QVector>

class iADockWidgetWrapper;

class iAParamFeaturesView;
class iAParamSPLOMView;
class iAParamSpatialView;
class iAParamTableView;

class QSettings;

class iAParameterExplorerAttachment : public iAModuleAttachmentToChild
{
public:
	static iAParameterExplorerAttachment* create(iAMainWindow * mainWnd, iAMdiChild * child);
	void LoadCSV(QString const & fileName);
	void ToggleDockWidgetTitleBars();
	void ToggleSettings(bool visible);
	void SaveAll(QString const & fileName);
	void SaveSettings(QSettings & settings);
	void LoadSettings(QSettings const & settings);
	QString const & CSVFileName() const;
private:
	iAParameterExplorerAttachment(iAMainWindow * mainWnd, iAMdiChild * child);
	iAParamSPLOMView* m_SPLOMView;
	iAParamSpatialView* m_spatialView;
	iAParamTableView* m_tableView;
	iAParamFeaturesView* m_featuresView;
	QVector<iADockWidgetWrapper*> m_dockWidgets;
	QString m_csvFileName;
};
