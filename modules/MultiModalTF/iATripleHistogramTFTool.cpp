// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATripleHistogramTFTool.h"

#include "tf_2mod/dlg_tf_2mod.h"
#include "tf_3mod/dlg_tf_3mod.h"

#include <iAMdiChild.h>

iATripleHistogramTFTool::iATripleHistogramTFTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_tf_2mod(nullptr),
	m_tf_3mod(nullptr)
{}

void iATripleHistogramTFTool::start2TF()
{
	if (!m_tf_2mod)
	{
		m_tf_2mod = new dlg_tf_2mod(m_child);
		m_child->tabifyDockWidget(m_child->renderDockWidget(), m_tf_2mod);
	}
	m_tf_2mod->show();
	m_tf_2mod->raise();
}

void iATripleHistogramTFTool::start3TF()
{
	if (!m_tf_3mod)
	{
		m_tf_3mod = new dlg_tf_3mod(m_child);
		m_child->tabifyDockWidget(m_child->renderDockWidget(), m_tf_3mod);
	}
	m_tf_3mod->show();
	m_tf_3mod->raise();
}
