// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALabellingTool.h"

#include "iALabelsDlg.h"

#include <iADockWidgetWrapper.h>
#include <iAMdiChild.h>

const QString iALabellingTool::Name("Labelling");

iALabellingTool::iALabellingTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child), m_dlgLabels(new iALabelsDlg(child))
{
	child->splitDockWidget(child->renderDockWidget(),
		new iADockWidgetWrapper(m_dlgLabels, "Labels", "LabelsDW", "https://github.com/3dct/open_iA/wiki/Labelling"),
		Qt::Vertical);
}

iALabelsDlg* iALabellingTool::labelsDlg()
{
	return m_dlgLabels;
}

void iALabellingTool::loadState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	m_dlgLabels->loadState(projectFile);
}

void iALabellingTool::saveState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	m_dlgLabels->saveState(projectFile);
}
