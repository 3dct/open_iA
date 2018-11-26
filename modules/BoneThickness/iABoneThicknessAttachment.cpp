/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iABoneThicknessChartBar.h"
#include "iABoneThicknessSplitter.h"
#include "iABoneThicknessTable.h"

#include "qthelper/iADockWidgetWrapper.h"
#include "iARenderer.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>

iABoneThicknessAttachment::iABoneThicknessAttachment(MainWindow* _pMainWnd, iAChildData _iaChildData):
	iAModuleAttachmentToChild(_pMainWnd, _iaChildData)
{
	QWidget* pWidget(new QWidget());

	m_pBoneThickness.reset(new iABoneThickness());
	m_pBoneThicknessChartBar = new iABoneThicknessChartBar(pWidget);
	m_pBoneThicknessTable = new iABoneThicknessTable(pWidget);
	
	m_pBoneThickness->set(m_childData.child->getRenderer(), m_childData.polyData, m_pBoneThicknessChartBar, m_pBoneThicknessTable);
	m_pBoneThicknessChartBar->set(m_pBoneThickness.data(), m_pBoneThicknessTable);
	m_pBoneThicknessTable->set(m_pBoneThickness.data(), m_pBoneThicknessChartBar);

	QPushButton* pPushButtonOpen(new QPushButton("Open control points file...", pWidget));
	pPushButtonOpen->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));
	connect(pPushButtonOpen, SIGNAL(clicked()), this, SLOT(slotPushButtonOpen()));

	QPushButton* pPushButtonSave(new QPushButton("Save table to file...", pWidget));
	pPushButtonSave->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
	connect(pPushButtonSave, SIGNAL(clicked()), this, SLOT(slotPushButtonSave()));

	QGroupBox* pGroupBoxBound(new QGroupBox("Surface bounds", pWidget));
	pGroupBoxBound->setFixedHeight(pGroupBoxBound->logicalDpiY() / 2);

	QLabel* pLabelBoundXMin(new QLabel(QString("X min: %1").arg(m_pBoneThickness->axisXMin()), pGroupBoxBound));
	QLabel* pLabelBoundXMax(new QLabel(QString("X max: %1").arg(m_pBoneThickness->axisXMax()), pGroupBoxBound));
	QLabel* pLabelBoundXRng(new QLabel(QString("X range: %1").arg(m_pBoneThickness->rangeX()), pGroupBoxBound));
	QLabel* pLabelBoundYMin(new QLabel(QString("Y min: %1").arg(m_pBoneThickness->axisYMin()), pGroupBoxBound));
	QLabel* pLabelBoundYMax(new QLabel(QString("Y max: %1").arg(m_pBoneThickness->axisYMax()), pGroupBoxBound));
	QLabel* pLabelBoundYRng(new QLabel(QString("Y range: %1").arg(m_pBoneThickness->rangeY()), pGroupBoxBound));
	QLabel* pLabelBoundZMin(new QLabel(QString("Z min: %1").arg(m_pBoneThickness->axisZMin()), pGroupBoxBound));
	QLabel* pLabelBoundZMax(new QLabel(QString("Z max: %1").arg(m_pBoneThickness->axisXMax()), pGroupBoxBound));
	QLabel* pLabelBoundZRng(new QLabel(QString("Z range: %1").arg(m_pBoneThickness->rangeZ()), pGroupBoxBound));

	QGridLayout* pGridLayoutBound(new QGridLayout(pGroupBoxBound));
	pGridLayoutBound->addWidget(pLabelBoundXMin, 0, 0);
	pGridLayoutBound->addWidget(pLabelBoundXMax, 0, 1);
	pGridLayoutBound->addWidget(pLabelBoundXRng, 0, 2);
	pGridLayoutBound->addWidget(pLabelBoundYMin, 0, 3);
	pGridLayoutBound->addWidget(pLabelBoundYMax, 0, 4);
	pGridLayoutBound->addWidget(pLabelBoundYRng, 0, 5);
	pGridLayoutBound->addWidget(pLabelBoundZMin, 0, 6);
	pGridLayoutBound->addWidget(pLabelBoundZMax, 0, 7);
	pGridLayoutBound->addWidget(pLabelBoundZRng, 0, 8);

	iABoneThicknessSplitter* pBoneThicknessSplitter(new iABoneThicknessSplitter(pWidget));
	pBoneThicknessSplitter->addWidget(m_pBoneThicknessTable);
	pBoneThicknessSplitter->addWidget(m_pBoneThicknessChartBar);

	QGroupBox* pGroupBoxSettings(new QGroupBox("Settings", pWidget));
	pGroupBoxSettings->setFixedHeight(pGroupBoxSettings->logicalDpiY() / 2);

	QLabel* pLabelSphereRadius(new QLabel("Calculation radius:", pGroupBoxSettings));
	m_pDoubleSpinBoxSphereRadius = new QDoubleSpinBox(pGroupBoxSettings);
	m_pDoubleSpinBoxSphereRadius->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxSphereRadius->setMinimum(0.01);
	m_pDoubleSpinBoxSphereRadius->setMaximum(1.0E+6);
	m_pDoubleSpinBoxSphereRadius->setSingleStep(0.1);
	m_pDoubleSpinBoxSphereRadius->setValue(m_pBoneThickness->sphereRadius());
	connect(m_pDoubleSpinBoxSphereRadius, SIGNAL(editingFinished()), this, SLOT(slotDoubleSpinBoxSphereRadius()));

	QLabel* pLabelThicknessMaximum(new QLabel("Maximum thickness:", pGroupBoxSettings));
	m_pDoubleSpinBoxThicknessMaximum = new QDoubleSpinBox(pGroupBoxSettings);
	m_pDoubleSpinBoxThicknessMaximum->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxThicknessMaximum->setMinimum(0.0);
	m_pDoubleSpinBoxThicknessMaximum->setSingleStep(1.0);
	m_pDoubleSpinBoxThicknessMaximum->setValue(m_pBoneThickness->thicknessMaximum());
	connect(m_pDoubleSpinBoxThicknessMaximum, SIGNAL(editingFinished()), this, SLOT(slotDoubleSpinBoxThicknessMaximum()));

	QCheckBox* pCheckBoxTransparency(new QCheckBox("Use transparency", pGroupBoxSettings));
	connect(pCheckBoxTransparency, SIGNAL(clicked(const bool&)), this, SLOT(slotCheckBoxTransparency(const bool&)));

	QCheckBox* pCheckBoxShowThickness(new QCheckBox("Thickness representation", pGroupBoxSettings));
	pCheckBoxShowThickness->setChecked(m_pBoneThickness->showThickness());
	connect(pCheckBoxShowThickness, SIGNAL(clicked(const bool&)), this, SLOT(slotCheckBoxShowThickness(const bool&)));

	QGridLayout* pGridLayoutSettings(new QGridLayout(pGroupBoxSettings));
	pGridLayoutSettings->addWidget(pLabelSphereRadius, 0, 0, Qt::AlignRight);
	pGridLayoutSettings->addWidget(m_pDoubleSpinBoxSphereRadius, 0, 1, Qt::AlignLeft);
	pGridLayoutSettings->addWidget(pLabelThicknessMaximum, 0, 2, Qt::AlignRight);
	pGridLayoutSettings->addWidget(m_pDoubleSpinBoxThicknessMaximum, 0, 3, Qt::AlignLeft);
	pGridLayoutSettings->addWidget(pCheckBoxTransparency, 0, 4);
	pGridLayoutSettings->addWidget(pCheckBoxShowThickness, 0, 5);

	QGridLayout* pGridLayout(new QGridLayout(pWidget));
	pGridLayout->addWidget(pPushButtonOpen, 0, 0);
	pGridLayout->addWidget(pPushButtonSave, 0, 1);
	pGridLayout->addWidget(pGroupBoxBound, 1, 0, 1, 2);
	pGridLayout->addWidget(pBoneThicknessSplitter, 2, 0, 1, 2);
	pGridLayout->addWidget(pGroupBoxSettings, 3, 0, 1, 2);

	iADockWidgetWrapper* pDockWidgetWrapper(new iADockWidgetWrapper(pWidget, tr("Bone thickness"), "BoneThickness"));
	_iaChildData.child->tabifyDockWidget(_iaChildData.logs, pDockWidgetWrapper);

	pDockWidgetWrapper->adjustSize();
	m_pBoneThicknessChartBar->resize(pBoneThicknessSplitter->width() / 2, pBoneThicknessSplitter->height());
}

void iABoneThicknessAttachment::slotCheckBoxShowThickness(const bool& _bChecked)
{
	m_pBoneThickness->setShowThickness(_bChecked);
	m_pBoneThickness->setWindowSpheres();
	m_childData.child->getRenderer()->update();
}

void iABoneThicknessAttachment::slotCheckBoxTransparency(const bool& _bChecked)
{
	m_pBoneThickness->setTransparency(_bChecked);
}

void iABoneThicknessAttachment::slotDoubleSpinBoxSphereRadius()
{
	const double dSphereRadius(m_pDoubleSpinBoxSphereRadius->value());

	if (m_pBoneThickness->sphereRadius() != dSphereRadius)
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		m_pBoneThickness->setSphereRadius(dSphereRadius);
		m_pBoneThickness->calculate();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindowSpheres();
		m_childData.child->getRenderer()->update();
		qApp->restoreOverrideCursor();
	}
}

void iABoneThicknessAttachment::slotDoubleSpinBoxThicknessMaximum()
{
	const double dThicknessMaximum(m_pDoubleSpinBoxThicknessMaximum->value());

	if (m_pBoneThickness->thicknessMaximum() != dThicknessMaximum)
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		m_pBoneThickness->setThicknessMaximum(dThicknessMaximum);
		m_pBoneThickness->calculate();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindow();
		qApp->restoreOverrideCursor();
	}
}

void iABoneThicknessAttachment::slotPushButtonOpen()
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
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pBoneThickness->open(pFileDialog->selectedFiles().first());
		m_pBoneThickness->calculate();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindow();
		qApp->restoreOverrideCursor();
	}

	delete pFileDialog;
}

void iABoneThicknessAttachment::slotPushButtonSave()
{
	QPushButton* pPushButtonSave((QPushButton*)sender());

	QFileDialog* pFileDialog(new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
	pFileDialog->setDefaultSuffix("csv");
	pFileDialog->setFileMode(QFileDialog::ExistingFile);
	pFileDialog->setNameFilter("CSV file (*.csv)");
	pFileDialog->setWindowTitle(pPushButtonSave->text());

	if (pFileDialog->exec())
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pBoneThickness->save(pFileDialog->selectedFiles().first());
		qApp->restoreOverrideCursor();
	}

	delete pFileDialog;
}
