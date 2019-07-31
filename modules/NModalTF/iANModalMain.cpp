/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iANModalMain.h"

#include "iAConsole.h"
#include "mdichild.h"


// Module interface and Attachment --------------------------------------------------------

iANModalAttachment::iANModalAttachment(MainWindow * mainWnd, MdiChild *child) :
	iAModuleAttachmentToChild(mainWnd, child),
	m_nModalMain(nullptr)
{
	// Do nothing
}

iANModalAttachment* iANModalAttachment::create(MainWindow * mainWnd, MdiChild *child) {
	auto newAttachment = new iANModalAttachment(mainWnd, child);
	return newAttachment;
}

void iANModalAttachment::start() {
	if (!m_nModalMain) {
		m_nModalMain = new iANModalMain(m_child);
		m_child->tabifyDockWidget(m_child->logDockWidget(), m_nModalMain);
	}
	m_nModalMain->show();
	m_nModalMain->raise();
}


// n-Modal Widget -------------------------------------------------------------------------

#include "iANModalWidget.h"

iANModalMain::iANModalMain(MdiChild *mdiChild):
	QDockWidget("n-Modal Transfer Function", mdiChild)
{
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);
	m_nModalWidget = new iANModalWidget(mdiChild);
	setWidget(m_nModalWidget);
}
iANModalWidget* iANModalMain::nModalWidget() {
	return m_nModalWidget;
}