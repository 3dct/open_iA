/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iAFoamCharacterizationModuleInterface.h"
#include "iAFoamCharacterizationAttachment.h"
#include "mainwindow.h"

#include <mdichild.h>

iAFoamCharacterizationModuleInterface::iAFoamCharacterizationModuleInterface( )
{ /* not implemented */ }

iAFoamCharacterizationModuleInterface::~iAFoamCharacterizationModuleInterface( )
{ /* not implemented */ }

void iAFoamCharacterizationModuleInterface::Initialize( )
{
	QMenu* toolsMenu (m_mainWnd->getToolsMenu());

	QAction* pFoamCharacterization(new QAction(QApplication::translate("MainWindows", "Foam characterization", 0), m_mainWnd));
	connect(pFoamCharacterization, SIGNAL(triggered()), this, SLOT(slotFoamCharacterization()));
	AddActionToMenuAlphabeticallySorted(toolsMenu, pFoamCharacterization);
}

void iAFoamCharacterizationModuleInterface::slotFoamCharacterization()
{
	PrepareActiveChild();

	if (m_mdiChild)
	{
		AttachToMdiChild(m_mdiChild);
	}
}

iAModuleAttachmentToChild* iAFoamCharacterizationModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData)
{
	return new iAFoamCharacterizationAttachment(mainWnd, childData);
}
