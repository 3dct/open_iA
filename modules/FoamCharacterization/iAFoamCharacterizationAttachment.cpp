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
#include "iAFoamCharacterizationAttachment.h"

#include "iAFoamCharacterizationDialogAnalysis.h"
#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationItemDistanceTransform.h"
#include "iAFoamCharacterizationItemFilter.h"
#include "iAFoamCharacterizationItemWatershed.h"
#include "iAFoamCharacterizationTable.h"

#include <mdichild.h>
#include <mainwindow.h>
#include <qthelper/iADockWidgetWrapper.h>

#include <vtkImageData.h>

#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>

iAFoamCharacterizationAttachment::iAFoamCharacterizationAttachment(MainWindow* mainWnd, MdiChild * child)
																			  : iAModuleAttachmentToChild(mainWnd, child)
																			  , m_pImageData(child->getImageData())
{
	m_pImageRestore = vtkImageData::New();
	m_pImageRestore->DeepCopy(m_pImageData);

	QWidget* pWidget(new QWidget());

	QGroupBox* pGroupBox1(new QGroupBox("Foam characterization", pWidget));

	QPushButton* pPushButtonOpen(new QPushButton("Open table...", pWidget));
	pPushButtonOpen->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));
	connect(pPushButtonOpen, SIGNAL(clicked()), this, SLOT(slotPushButtonOpen()));

	QPushButton* pPushButtonSave(new QPushButton("Save table...", pWidget));
	pPushButtonSave->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
	connect(pPushButtonSave, SIGNAL(clicked()), this, SLOT(slotPushButtonSave()));

	QPushButton* pPushButtonClear(new QPushButton("Clear table...", pWidget));
	pPushButtonClear->setIcon(qApp->style()->standardIcon(QStyle::SP_FileIcon));
	connect(pPushButtonClear, SIGNAL(clicked()), this, SLOT(slotPushButtonClear()));

	QPushButton* pPushButtonFilter(new QPushButton("Add filter", pWidget));
	iAFoamCharacterizationItemFilter itemFilter(0, m_pImageData);
	pPushButtonFilter->setIcon(itemFilter.itemButtonIcon());
	connect(pPushButtonFilter, SIGNAL(clicked()), this, SLOT(slotPushButtonFilter()));

	QPushButton* pPushButtonBinarization(new QPushButton("Add binarization", pWidget));
	iAFoamCharacterizationItemBinarization itemBinarization(0, m_pImageData);
	pPushButtonBinarization->setIcon(itemBinarization.itemButtonIcon());
	connect(pPushButtonBinarization, SIGNAL(clicked()), this, SLOT(slotPushButtonBinarization()));

	QPushButton* pPushButtonDistanceTransform(new QPushButton("Add distance transform", pWidget));
	iAFoamCharacterizationItemDistanceTransform itemDistanceTransform(0, m_pImageData);
	pPushButtonDistanceTransform->setIcon(itemDistanceTransform.itemButtonIcon());
	connect(pPushButtonDistanceTransform, SIGNAL(clicked()), this, SLOT(slotPushButtonDistanceTransform()));

	QPushButton* pPushButtonWatershed(new QPushButton("Add watershed", pWidget));
	iAFoamCharacterizationItemWatershed itemWatershed(0, m_pImageData);
	pPushButtonWatershed->setIcon(itemWatershed.itemButtonIcon());
	connect(pPushButtonWatershed, SIGNAL(clicked()), this, SLOT(slotPushButtonWatershed()));

	m_pTable = new iAFoamCharacterizationTable(m_pImageData, pWidget);

	QPushButton* pPushButtonExecute(new QPushButton("Execute", pWidget));
	pPushButtonExecute->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogApplyButton));
	connect(pPushButtonExecute, SIGNAL(clicked()), this, SLOT(slotPushButtonExecute()));

	m_pPushButtonAnalysis = new QPushButton("Analysis", pWidget);
	m_pPushButtonAnalysis->setIcon(qApp->style()->standardIcon(QStyle::SP_FileDialogStart));
	m_pPushButtonAnalysis->setEnabled(false);
	connect(m_pPushButtonAnalysis, SIGNAL(clicked()), this, SLOT(slotPushButtonAnalysis()));

	QPushButton* pPushButtonRestore(new QPushButton("Restore image", pWidget));
	pPushButtonRestore->setIcon(qApp->style()->standardIcon(QStyle::SP_DriveHDIcon));
	connect(pPushButtonRestore, SIGNAL(clicked()), this, SLOT(slotPushButtonRestore()));

	QGridLayout* pGridLayout1(new QGridLayout(pGroupBox1));
	pGridLayout1->addWidget(pPushButtonOpen, 0, 0);
	pGridLayout1->addWidget(pPushButtonSave, 0, 1);
	pGridLayout1->addWidget(pPushButtonClear, 0, 2);
	pGridLayout1->addWidget(pPushButtonFilter, 0, 3);
	pGridLayout1->addWidget(pPushButtonBinarization, 0, 4);
	pGridLayout1->addWidget(pPushButtonDistanceTransform, 0, 5);
	pGridLayout1->addWidget(pPushButtonWatershed, 0, 6);
	pGridLayout1->addWidget(m_pTable, 1, 0, 1, 7);
	pGridLayout1->addWidget(  pPushButtonExecute, 2, 0);
	pGridLayout1->addWidget(m_pPushButtonAnalysis, 2, 1);
	pGridLayout1->addWidget(  pPushButtonRestore, 2, 6);

	QGridLayout* pGridLayout(new QGridLayout(pWidget));
	pGridLayout->addWidget(pGroupBox1);

	iADockWidgetWrapper* pDockWidgetWrapper(new iADockWidgetWrapper(pWidget, tr("Foam characterization"), "FoamCharacterization"));
	child->tabifyDockWidget(child->getLogDlg(), pDockWidgetWrapper);
}

void iAFoamCharacterizationAttachment::slotPushButtonAnalysis()
{
	iAFoamCharacterizationDialogAnalysis* pDialogAnalysis (new iAFoamCharacterizationDialogAnalysis(m_pImageData, m_mainWnd));
	pDialogAnalysis->show();
}

void iAFoamCharacterizationAttachment::slotPushButtonBinarization()
{
	m_pTable->addBinarization();
}

void iAFoamCharacterizationAttachment::slotPushButtonClear()
{
	if ( QMessageBox::question ( m_child, "Question", "Clear table? All items will be removed."
							   , QMessageBox::Yes, QMessageBox::No
							   ) == QMessageBox::Yes
	   )
	{
		m_pTable->clear();
	}
}

void iAFoamCharacterizationAttachment::slotPushButtonDistanceTransform()
{
	m_pTable->addDistanceTransform();
}

void iAFoamCharacterizationAttachment::slotPushButtonExecute()
{
	if ( QMessageBox::question(m_child, "Question", "Execute foam characterization pipeline?", QMessageBox::Yes, QMessageBox::No)
	     == QMessageBox::Yes
	   )
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pTable->execute();
		m_child->enableRenderWindows();
		qApp->restoreOverrideCursor();

		m_pPushButtonAnalysis->setEnabled(true);
	}
}

void iAFoamCharacterizationAttachment::slotPushButtonFilter()
{
	m_pTable->addFilter();
}

void iAFoamCharacterizationAttachment::slotPushButtonOpen()
{
	QPushButton* pPushButtonOpen((QPushButton*)sender());

	QScopedPointer<QFileDialog> pFileDialog(new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
	pFileDialog->setDefaultSuffix("fch");
	pFileDialog->setFileMode(QFileDialog::ExistingFile);
	pFileDialog->setNameFilter("Foam characterization table file (*.fch)");
	pFileDialog->setWindowTitle(pPushButtonOpen->text());

	if (pFileDialog->exec())
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pTable->open(pFileDialog->selectedFiles().first());
		qApp->restoreOverrideCursor();
	}
}

void iAFoamCharacterizationAttachment::slotPushButtonRestore()
{
	if ( QMessageBox::question(m_child, "Question", "Restore original image?", QMessageBox::Yes, QMessageBox::No)
		 == QMessageBox::Yes
	   )
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pImageData->DeepCopy(m_pImageRestore);
		m_pTable->reset();
		m_child->enableRenderWindows();
		qApp->restoreOverrideCursor();
	}
}

void iAFoamCharacterizationAttachment::slotPushButtonSave()
{
	QPushButton* pPushButtonSave((QPushButton*)sender());

	QFileDialog* pFileDialog(new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
	pFileDialog->setDefaultSuffix("fch");
	pFileDialog->setNameFilter("Foam characterization table file (*.fch)");
	pFileDialog->setWindowTitle(pPushButtonSave->text());

	if (pFileDialog->exec())
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pTable->save(pFileDialog->selectedFiles().first());
		qApp->restoreOverrideCursor();
	}
}

void iAFoamCharacterizationAttachment::slotPushButtonWatershed()
{
	m_pTable->addWatershed();
}
