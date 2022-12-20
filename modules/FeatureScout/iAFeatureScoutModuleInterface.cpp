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
#include "iAFeatureScoutModuleInterface.h"

#include "iAFeatureScoutTool.h"

#include <dlg_CSVInput.h>

#include <iAFileUtils.h>
#include <iALog.h>
#include <iAToolRegistry.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <vtkTable.h>

#include <QFile>
#include <QMenu>
#include <QMessageBox>

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(FeatureScout);

	iAToolRegistry::addTool(iAFeatureScoutTool::ID, iAFeatureScoutTool::create);
	QAction * actionFibreScout = new QAction(tr("FeatureScout"), m_mainWnd);
	connect(actionFibreScout, &QAction::triggered, this, &iAFeatureScoutModuleInterface::featureScout);
	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Feature Analysis"), true);
	submenu->addAction(actionFibreScout);
}

void iAFeatureScoutModuleInterface::featureScout()
{
	bool volumeDataAvailable = m_mainWnd->activeMdiChild() &&
		m_mainWnd->activeMdiChild()->firstImageData() != nullptr;
	dlg_CSVInput dlg(volumeDataAvailable);
	if (m_mainWnd->activeMdiChild())
	{
		auto mdi = m_mainWnd->activeMdiChild();
		QString testCSVFileName = pathFileBaseName(mdi->fileInfo()) + ".csv";
		if (QFile(testCSVFileName).exists())
		{
			dlg.setFileName(testCSVFileName);
			auto type = iAFeatureScoutTool::guessFeatureType(testCSVFileName);
			if (type != InvalidObjectType)
			{
				dlg.setFormat(type == Voids ? iACsvConfig::FCVoidFormat : iACsvConfig::FCPFiberFormat);
			}
		}
		else
		{
			dlg.setPath(mdi->filePath());
		}
	}
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	iACsvConfig csvConfig = dlg.getConfig();
	bool createdMdi = false;
	iAMdiChild* child = nullptr;
	if (csvConfig.visType != iACsvConfig::UseVolume)
	{
		if (m_mainWnd->activeMdiChild() && QMessageBox::question(m_mainWnd, "FeatureScout",
			"Load FeatureScout in currently active window (If you choose No, FeatureScout will be opened in a new window)?",
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			child = m_mainWnd->activeMdiChild();
		}
		else
		{
			createdMdi = true;
			child = m_mainWnd->createMdiChild(false);
		}
	}
	else
	{
		child = m_mainWnd->activeMdiChild();
	}
	if (!iAFeatureScoutTool::addToChild(child, csvConfig))
	{
		QMessageBox::warning(m_mainWnd, "FeatureScout",
			"Starting FeatureScout failed! Please check console for detailed error messages!");
		if (createdMdi)
		{
			m_mainWnd->closeMdiChild(child);
		}
	}
}
