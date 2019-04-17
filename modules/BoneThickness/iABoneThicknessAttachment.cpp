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
#include "iABoneThicknessAttachment.h"

#include "iABoneThicknessChartBar.h"
#include "iABoneThicknessSplitter.h"
#include "iABoneThicknessTable.h"

#include <qthelper/iADockWidgetWrapper.h>
#include <iARenderer.h>
#include <mdichild.h>
#include <mainwindow.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

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

	QGroupBox* pGroupBoxBound(new QGroupBox("Model Statistics", pWidget));
	pGroupBoxBound->setFixedHeight(pGroupBoxBound->logicalDpiY() / 2);

	pLabelMeanTh = new QLabel(QString("Thickness Mean: %1").arg(m_pBoneThickness->meanThickness()), pGroupBoxBound);
	pLabelStdTh = new QLabel(QString("Thickness STD: %1").arg(m_pBoneThickness->stdThickness()), pGroupBoxBound);
	pLabelMeanSDi = new QLabel(QString("Surface Distance Mean: %1").arg(m_pBoneThickness->meanSurfaceDistance()), pGroupBoxBound);
	pLabelStdSDi = new QLabel(QString("Surface Distance STD: %1").arg(m_pBoneThickness->stdSurfaceDistance()), pGroupBoxBound);

	QGridLayout* pGridLayoutBound(new QGridLayout(pGroupBoxBound));
	pGridLayoutBound->addWidget(pLabelMeanTh, 0, 0);
	pGridLayoutBound->addWidget(pLabelStdTh, 0, 1);
	pGridLayoutBound->addWidget(pLabelMeanSDi, 0, 2);
	pGridLayoutBound->addWidget(pLabelStdSDi, 0, 3);

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

	QLabel* pLabelSurfaceDistanceMaximum(new QLabel("Maximum surface distance:", pGroupBoxSettings));
	m_pDoubleSpinBoxSurfaceDistanceMaximum = new QDoubleSpinBox(pGroupBoxSettings);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setMinimum(0.0);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setSingleStep(1.0);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setValue(m_pBoneThickness->surfaceDistanceMaximum());
	connect(m_pDoubleSpinBoxSurfaceDistanceMaximum, SIGNAL(editingFinished()), this, SLOT(slotDoubleSpinBoxSurfaceDistanceMaximum()));


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
	pGridLayoutSettings->addWidget(pLabelSurfaceDistanceMaximum, 0, 4, Qt::AlignRight);
	pGridLayoutSettings->addWidget(m_pDoubleSpinBoxSurfaceDistanceMaximum, 0, 5, Qt::AlignLeft);
	pGridLayoutSettings->addWidget(pCheckBoxTransparency, 0, 6);
	pGridLayoutSettings->addWidget(pCheckBoxShowThickness, 0, 7);

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

void iABoneThicknessAttachment::setStatistics() {
	iABoneThicknessAttachment::pLabelMeanTh->setText(QString("Thickness Mean: %1").arg(m_pBoneThickness->meanThickness()));
	iABoneThicknessAttachment::pLabelStdTh->setText(QString("Thickness STD: %1").arg(m_pBoneThickness->stdThickness()));
	iABoneThicknessAttachment::pLabelMeanSDi->setText(QString("Surface Distance Mean: %1").arg(m_pBoneThickness->meanSurfaceDistance()));
	iABoneThicknessAttachment::pLabelStdSDi->setText(QString("Surface Distance STD: %1").arg(m_pBoneThickness->stdSurfaceDistance()));
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
		setStatistics();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindowSpheres();
		m_childData.child->getRenderer()->update();
		qApp->restoreOverrideCursor();
	}
}

void iABoneThicknessAttachment::slotDoubleSpinBoxSurfaceDistanceMaximum() {

	const double dSurfaceDistanceMaximum(m_pDoubleSpinBoxSurfaceDistanceMaximum->value());

	if (m_pBoneThickness->surfaceDistanceMaximum() != dSurfaceDistanceMaximum)
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		m_pBoneThickness->setSurfaceDistanceMaximum(dSurfaceDistanceMaximum);
		m_pBoneThickness->calculate();
		setStatistics();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindow();
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
		setStatistics();
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
		setStatistics();
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
