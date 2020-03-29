/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iATripleHistogramTFAttachment.h"

#include "tf_2mod/dlg_tf_2mod.h"
#include "tf_3mod/dlg_tf_3mod.h"

#include <mdichild.h>

iATripleHistogramTFAttachment::iATripleHistogramTFAttachment(MainWindow * mainWnd, MdiChild* child) :
	iAModuleAttachmentToChild(mainWnd, child),
	m_tf_2mod(nullptr),
	m_tf_3mod(nullptr)
{}

iATripleHistogramTFAttachment* iATripleHistogramTFAttachment::create(MainWindow * mainWnd, MdiChild* child)
{
	auto newAttachment = new iATripleHistogramTFAttachment(mainWnd, child);
	return newAttachment;
}

void iATripleHistogramTFAttachment::start2TF()
{
	if (!m_tf_2mod)
	{
		m_tf_2mod = new dlg_tf_2mod(m_child);
		m_child->tabifyDockWidget(m_child->logDockWidget(), m_tf_2mod);
	}
	m_tf_2mod->show();
	m_tf_2mod->raise();
}

void iATripleHistogramTFAttachment::start3TF()
{
	if (!m_tf_3mod)
	{
		m_tf_3mod = new dlg_tf_3mod(m_child);
		m_child->tabifyDockWidget(m_child->logDockWidget(), m_tf_3mod);
	}
	m_tf_3mod->show();
	m_tf_3mod->raise();
}
