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

#include "iABoneThicknessAttachment.h"

#include "mdichild.h"
#include "mainwindow.h"

#include"vtkPolyData.h"

#include "iADockWidgetWrapper.h"

#include <QPushButton>
#include <QFileDialog>

iABoneThicknessAttachment::iABoneThicknessAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData)
{
	QWidget* pBoneThicknessWidget (new QWidget());

	m_pBoneThicknessTable = new iABoneThicknessTable(pBoneThicknessWidget);

	const int iPushButtonWidth (2 * pBoneThicknessWidget->logicalDpiX());

	QPushButton* pPushButtonBoneThicknessOpen(new QPushButton("Open point file...", pBoneThicknessWidget));
	pPushButtonBoneThicknessOpen->setFixedWidth(iPushButtonWidth);
	connect(pPushButtonBoneThicknessOpen, SIGNAL(clicked()), this, SLOT(slotPushButtonBoneThicknessOpen()));
	/*
	QPushButton* pPushButtonBoneThicknessCalculate(new QPushButton("Calculate thickness", pBoneThicknessWidget));
	pPushButtonBoneThicknessCalculate->setFixedWidth(iPushButtonWidth);
	connect(pPushButtonBoneThicknessCalculate, SIGNAL(clicked()), this, SLOT(slotPushButtonBoneThicknessCalculate()));
	*/
	QGridLayout* pBoneThicknessLayout (new QGridLayout(pBoneThicknessWidget));
	pBoneThicknessLayout->addWidget(m_pBoneThicknessTable            , 0, 0, 3, 0);
	pBoneThicknessLayout->addWidget(pPushButtonBoneThicknessOpen     , 0, 1, Qt::AlignLeft);
	//pBoneThicknessLayout->addWidget(pPushButtonBoneThicknessCalculate, 1, 1, Qt::AlignLeft);

	childData.child->tabifyDockWidget(childData.logs, new iADockWidgetWrapper(pBoneThicknessWidget, tr("Bone thickness"), "BoneThickness"));
}

void iABoneThicknessAttachment::slotPushButtonBoneThicknessCalculate()
{
	QVector<float>* pvThickness (m_pBoneThicknessTable->thickness());

	vtkPolyData* polyData (m_childData.polyData);
	vtkCellArray* cellArray (polyData->GetPolys());

	const vtkIdType numberCells (cellArray->GetNumberOfCells());

	for (int i (0) ; i < pvThickness->size() ; ++i)
	{
		(*pvThickness)[i] = static_cast<float> (i);
	}

	m_pBoneThicknessTable->setTable();
}

void iABoneThicknessAttachment::slotPushButtonBoneThicknessOpen()
{
	QPushButton* pPushButtonOpen ((QPushButton*) sender());

	QFileDialog* pFileDialog (new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
	pFileDialog->setDefaultSuffix("txt");
	pFileDialog->setFileMode(QFileDialog::ExistingFile);
	pFileDialog->setNameFilter("Point file (*.txt)");
	pFileDialog->setWindowTitle(pPushButtonOpen->text());

	if (pFileDialog->exec())
	{
		m_pBoneThicknessTable->open(pFileDialog->selectedFiles().first());
		m_pBoneThicknessTable->setWindow(m_childData.child->getRaycaster());
	}

	delete pFileDialog;
}
