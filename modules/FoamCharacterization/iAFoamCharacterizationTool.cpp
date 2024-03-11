// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationTool.h"

#include "iAFoamCharacterizationDialogAnalysis.h"
#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationItemDistanceTransform.h"
#include "iAFoamCharacterizationItemFilter.h"
#include "iAFoamCharacterizationItemWatershed.h"
#include "iAFoamCharacterizationTable.h"

#include <iAMdiChild.h>
#include <iAMainWindow.h>

#include <iADockWidgetWrapper.h>

#include <iAImageData.h>

#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>

iAFoamCharacterizationTool::iAFoamCharacterizationTool(iAMainWindow* mainWnd, iAMdiChild * child)
																			  : iATool(mainWnd, child)
																			  , m_origDataSet(child->dataSet(child->firstImageDataSetIdx()))
{
	QWidget* pWidget(new QWidget());

	QGroupBox* pGroupBox1(new QGroupBox("Foam characterization", pWidget));

	QPushButton* pPushButtonOpen(new QPushButton("Open table...", pWidget));
	pPushButtonOpen->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
	connect(pPushButtonOpen, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonOpen);

	QPushButton* pPushButtonSave(new QPushButton("Save table...", pWidget));
	pPushButtonSave->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
	connect(pPushButtonSave, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonSave);

	QPushButton* pPushButtonClear(new QPushButton("Clear table...", pWidget));
	pPushButtonClear->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
	connect(pPushButtonClear, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonClear);

	QPushButton* pPushButtonFilter(new QPushButton("Add filter", pWidget));
	pPushButtonFilter->setIcon(iAFoamCharacterizationItemFilter::itemButtonIcon(iAFoamCharacterizationItem::itFilter));
	connect(pPushButtonFilter, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonFilter);

	QPushButton* pPushButtonBinarization(new QPushButton("Add binarization", pWidget));
	pPushButtonBinarization->setIcon(iAFoamCharacterizationItemBinarization::itemButtonIcon(iAFoamCharacterizationItem::itBinarization));
	connect(pPushButtonBinarization, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonBinarization);

	QPushButton* pPushButtonDistanceTransform(new QPushButton("Add distance transform", pWidget));
	pPushButtonDistanceTransform->setIcon(iAFoamCharacterizationItemDistanceTransform::itemButtonIcon(iAFoamCharacterizationItem::itDistanceTransform));
	connect(pPushButtonDistanceTransform, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonDistanceTransform);

	QPushButton* pPushButtonWatershed(new QPushButton("Add watershed", pWidget));
	pPushButtonWatershed->setIcon(iAFoamCharacterizationItemWatershed::itemButtonIcon(iAFoamCharacterizationItem::itWatershed));
	connect(pPushButtonWatershed, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonWatershed);

	m_pTable = new iAFoamCharacterizationTable(child, pWidget);

	QPushButton* pPushButtonExecute(new QPushButton("Execute", pWidget));
	pPushButtonExecute->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton));
	connect(pPushButtonExecute, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonExecute);

	m_pPushButtonAnalysis = new QPushButton("Analysis", pWidget);
	m_pPushButtonAnalysis->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogStart));
	m_pPushButtonAnalysis->setEnabled(false);
	connect(m_pPushButtonAnalysis, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonAnalysis);

	QPushButton* pPushButtonRestore(new QPushButton("Restore image", pWidget));
	pPushButtonRestore->setIcon(QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon));
	connect(pPushButtonRestore, &QPushButton::clicked, this, &iAFoamCharacterizationTool::slotPushButtonRestore);

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
	child->tabifyDockWidget(child->renderDockWidget(), pDockWidgetWrapper);
}

void iAFoamCharacterizationTool::slotPushButtonAnalysis()
{
	auto pDialogAnalysis = new iAFoamCharacterizationDialogAnalysis(dynamic_cast<iAImageData*>(m_child->dataSet(m_child->firstImageDataSetIdx()).get()), m_mainWindow);
	pDialogAnalysis->show();
}

void iAFoamCharacterizationTool::slotPushButtonBinarization()
{
	m_pTable->addBinarization();
}

void iAFoamCharacterizationTool::slotPushButtonClear()
{
	if ( QMessageBox::question ( m_child, "Question", "Clear table? All items will be removed."
							   , QMessageBox::Yes, QMessageBox::No
							   ) == QMessageBox::Yes
	   )
	{
		m_pTable->clear();
	}
}

void iAFoamCharacterizationTool::slotPushButtonDistanceTransform()
{
	m_pTable->addDistanceTransform();
}

void iAFoamCharacterizationTool::slotPushButtonExecute()
{
	if ( QMessageBox::question(m_child, "Question", "Execute foam characterization pipeline?", QMessageBox::Yes, QMessageBox::No)
	     == QMessageBox::Yes
	   )
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		m_pTable->execute();
		// TODO NEWIO
		// some update? used to be m_child->enableRenderWindows();
		QApplication::restoreOverrideCursor();
		m_pPushButtonAnalysis->setEnabled(true);
	}
}

void iAFoamCharacterizationTool::slotPushButtonFilter()
{
	m_pTable->addFilter();
}

void iAFoamCharacterizationTool::slotPushButtonOpen()
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
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		m_pTable->open(pFileDialog->selectedFiles().first());
		QApplication::restoreOverrideCursor();
	}
}

void iAFoamCharacterizationTool::slotPushButtonRestore()
{
	if ( QMessageBox::question(m_child, "Question", "Restore original image?", QMessageBox::Yes, QMessageBox::No)
		 == QMessageBox::Yes
	   )
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		m_child->clearDataSets();
		m_child->addDataSet(m_origDataSet);
		// TODO NEWIO
		// some update? used to be m_child->enableRenderWindows();
		m_pTable->reset();
		QApplication::restoreOverrideCursor();
	}
}

void iAFoamCharacterizationTool::slotPushButtonSave()
{
	QPushButton* pPushButtonSave((QPushButton*)sender());

	QFileDialog* pFileDialog(new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
	pFileDialog->setDefaultSuffix("fch");
	pFileDialog->setNameFilter("Foam characterization table file (*.fch)");
	pFileDialog->setWindowTitle(pPushButtonSave->text());

	if (pFileDialog->exec())
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		m_pTable->save(pFileDialog->selectedFiles().first());
		QApplication::restoreOverrideCursor();
	}
}

void iAFoamCharacterizationTool::slotPushButtonWatershed()
{
	m_pTable->addWatershed();
}
