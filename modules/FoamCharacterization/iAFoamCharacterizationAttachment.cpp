/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAFoamCharacterizationAttachment.h"

#include "mdichild.h"
#include "mainwindow.h"

#include <iADockWidgetWrapper.h>

#include <QGroupBox>
#include <QGridlayout>

#include <vtkImageData.h>

#include "iAFoamCharacterizationTable.h"

iAFoamCharacterizationAttachment::iAFoamCharacterizationAttachment(MainWindow* _pMainWnd, iAChildData _iaChildData) : iAModuleAttachmentToChild(_pMainWnd, _iaChildData)
																													, m_pImageData (_iaChildData.imgData)
{
	QWidget* pWidget(new QWidget());

	QGroupBox* pGroupBox1(new QGroupBox("Foam characterization", pWidget));

	m_pTable = new iAFoamCharacterizationTable(pWidget);

	QGridLayout* pGridLayout1(new QGridLayout(pGroupBox1));
	pGridLayout1->addWidget(m_pTable);

	QGridLayout* pGridLayout(new QGridLayout(pWidget));
	pGridLayout->addWidget(pGroupBox1);

	iADockWidgetWrapper* pDockWidgetWrapper(new iADockWidgetWrapper(pWidget, tr("Foam characterization"), "FoamCharacterization"));
	_iaChildData.child->tabifyDockWidget(_iaChildData.logs, pDockWidgetWrapper);
}
