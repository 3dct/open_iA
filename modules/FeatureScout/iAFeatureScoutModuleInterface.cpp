// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureScoutModuleInterface.h"

#include "iAFeatureScoutTool.h"

#include <dlg_CSVInput.h>

#include <iAFileUtils.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAToolRegistry.h>

#include <iALog.h>

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

	iAToolRegistry::addTool(iAFeatureScoutTool::ID, createTool<iAFeatureScoutTool>);
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
	if (csvConfig.visType != iAObjectVisType::UseVolume)
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
