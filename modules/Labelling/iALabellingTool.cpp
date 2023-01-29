// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALabellingTool.h"

#include "iALabelsDlg.h"

#include <iAMdiChild.h>

const QString iALabellingTool::Name("Labelling");

iALabellingTool::iALabellingTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child), m_dlgLabels(new iALabelsDlg(child))
{
	child->splitDockWidget(child->renderDockWidget(), m_dlgLabels, Qt::Vertical);
}

iALabelsDlg* iALabellingTool::labelsDlg()
{
	return m_dlgLabels;
}
