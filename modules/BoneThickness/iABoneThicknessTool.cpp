// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABoneThicknessTool.h"

#include "iABoneThicknessChartBar.h"
#include "iABoneThicknessSplitter.h"
#include "iABoneThicknessTable.h"

#include <iADockWidgetWrapper.h>

#include <iARenderer.h>
#include <iAMdiChild.h>
#include <iAMainWindow.h>

#include <iAPolyData.h>
#include <iALog.h>

#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include <vtkPolyData.h>

iABoneThicknessTool::iABoneThicknessTool(iAMainWindow* mainWnd, iAMdiChild * child):
	iATool(mainWnd, child)
{
	QWidget* pWidget(new QWidget());

	m_pBoneThickness.reset(new iABoneThickness());
	m_pBoneThicknessChartBar = new iABoneThicknessChartBar(pWidget);
	m_pBoneThicknessTable = new iABoneThicknessTable(pWidget);

	// TODO NEWIO: check
	vtkPolyData* pd = nullptr;
	for (auto ds : child->dataSetMap())
	{
		if (dynamic_cast<iAPolyData*>(ds.second.get()))
		{
			pd = dynamic_cast<iAPolyData*>(ds.second.get())->poly();
			break;
		}
	}
	if (!pd)
	{
		LOG(lvlError, "No mesh dataset loaded; but the BoneThickness tool requires a mesh dataset!");
		return;
	}
	m_pBoneThickness->set(m_child->renderer(), pd, m_pBoneThicknessChartBar, m_pBoneThicknessTable);
	m_pBoneThicknessChartBar->set(m_pBoneThickness.data(), m_pBoneThicknessTable);
	m_pBoneThicknessTable->set(m_pBoneThickness.data(), m_pBoneThicknessChartBar);

	QPushButton* pPushButtonOpen(new QPushButton("Open control points file...", pWidget));
	pPushButtonOpen->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
	connect(pPushButtonOpen, &QPushButton::clicked, this, &iABoneThicknessTool::slotPushButtonOpen);

	QPushButton* pPushButtonSave(new QPushButton("Save table to file...", pWidget));
	pPushButtonSave->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
	connect(pPushButtonSave, &QPushButton::clicked, this, &iABoneThicknessTool::slotPushButtonSave);

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
	connect(m_pDoubleSpinBoxSphereRadius, &QDoubleSpinBox::editingFinished, this, &iABoneThicknessTool::slotDoubleSpinBoxSphereRadius);

	QLabel* pLabelThicknessMaximum(new QLabel("Maximum thickness:", pGroupBoxSettings));
	m_pDoubleSpinBoxThicknessMaximum = new QDoubleSpinBox(pGroupBoxSettings);
	m_pDoubleSpinBoxThicknessMaximum->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxThicknessMaximum->setMinimum(0.0);
	m_pDoubleSpinBoxThicknessMaximum->setSingleStep(1.0);
	m_pDoubleSpinBoxThicknessMaximum->setValue(m_pBoneThickness->thicknessMaximum());
	connect(m_pDoubleSpinBoxThicknessMaximum, &QDoubleSpinBox::editingFinished, this, &iABoneThicknessTool::slotDoubleSpinBoxThicknessMaximum);

	QLabel* pLabelSurfaceDistanceMaximum(new QLabel("Maximum surface distance:", pGroupBoxSettings));
	m_pDoubleSpinBoxSurfaceDistanceMaximum = new QDoubleSpinBox(pGroupBoxSettings);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setMinimum(0.0);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setSingleStep(1.0);
	m_pDoubleSpinBoxSurfaceDistanceMaximum->setValue(m_pBoneThickness->surfaceDistanceMaximum());
	connect(m_pDoubleSpinBoxSurfaceDistanceMaximum, &QDoubleSpinBox::editingFinished, this, &iABoneThicknessTool::slotDoubleSpinBoxSurfaceDistanceMaximum);


	QCheckBox* pCheckBoxTransparency(new QCheckBox("Use transparency", pGroupBoxSettings));
	connect(pCheckBoxTransparency, &QCheckBox::clicked, this, &iABoneThicknessTool::slotCheckBoxTransparency);

	QCheckBox* pCheckBoxShowThickness(new QCheckBox("Thickness representation", pGroupBoxSettings));
	pCheckBoxShowThickness->setChecked(m_pBoneThickness->showThickness());
	connect(pCheckBoxShowThickness, &QCheckBox::clicked, this, &iABoneThicknessTool::slotCheckBoxShowThickness);

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

	iADockWidgetWrapper* pDockWidgetWrapper(new iADockWidgetWrapper(pWidget, tr("Bone thickness"), "BoneThickness", "https://github.com/3dct/open_iA/wiki/Tools"));
	m_child->tabifyDockWidget(m_child->renderDockWidget(), pDockWidgetWrapper);

	pDockWidgetWrapper->adjustSize();
	m_pBoneThicknessChartBar->resize(pBoneThicknessSplitter->width() / 2, pBoneThicknessSplitter->height());
}

void iABoneThicknessTool::setStatistics() {
	pLabelMeanTh->setText(QString("Thickness Mean: %1").arg(m_pBoneThickness->meanThickness()));
	pLabelStdTh->setText(QString("Thickness STD: %1").arg(m_pBoneThickness->stdThickness()));
	pLabelMeanSDi->setText(QString("Surface Distance Mean: %1").arg(m_pBoneThickness->meanSurfaceDistance()));
	pLabelStdSDi->setText(QString("Surface Distance STD: %1").arg(m_pBoneThickness->stdSurfaceDistance()));
}

void iABoneThicknessTool::slotCheckBoxShowThickness(const bool& _bChecked)
{
	m_pBoneThickness->setShowThickness(_bChecked);
	m_pBoneThickness->setWindowSpheres();
	m_child->renderer()->update();
}

void iABoneThicknessTool::slotCheckBoxTransparency(const bool& _bChecked)
{
	m_pBoneThickness->setTransparency(_bChecked);
}

void iABoneThicknessTool::slotDoubleSpinBoxSphereRadius()
{
	const double dSphereRadius(m_pDoubleSpinBoxSphereRadius->value());

	if (m_pBoneThickness->sphereRadius() != dSphereRadius)
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_pBoneThickness->setSphereRadius(dSphereRadius);
		m_pBoneThickness->calculate();
		setStatistics();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindowSpheres();
		m_child->renderer()->update();
		QApplication::restoreOverrideCursor();
	}
}

void iABoneThicknessTool::slotDoubleSpinBoxSurfaceDistanceMaximum() {

	const double dSurfaceDistanceMaximum(m_pDoubleSpinBoxSurfaceDistanceMaximum->value());

	if (m_pBoneThickness->surfaceDistanceMaximum() != dSurfaceDistanceMaximum)
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_pBoneThickness->setSurfaceDistanceMaximum(dSurfaceDistanceMaximum);
		m_pBoneThickness->calculate();
		setStatistics();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindow();
		QApplication::restoreOverrideCursor();
	}
}

void iABoneThicknessTool::slotDoubleSpinBoxThicknessMaximum()
{
	const double dThicknessMaximum(m_pDoubleSpinBoxThicknessMaximum->value());

	if (m_pBoneThickness->thicknessMaximum() != dThicknessMaximum)
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_pBoneThickness->setThicknessMaximum(dThicknessMaximum);
		m_pBoneThickness->calculate();
		setStatistics();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindow();
		QApplication::restoreOverrideCursor();
	}
}

void iABoneThicknessTool::slotPushButtonOpen()
{
	QPushButton* pPushButtonOpen ((QPushButton*) sender());

	QFileDialog pFileDialog;
	pFileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	pFileDialog.setDefaultSuffix("txt");
	pFileDialog.setFileMode(QFileDialog::ExistingFile);
	pFileDialog.setNameFilter("Point file (*.txt)");
	pFileDialog.setWindowTitle(pPushButtonOpen->text());

	if (pFileDialog.exec())
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		m_pBoneThickness->open(pFileDialog.selectedFiles().first());
		m_pBoneThickness->calculate();
		setStatistics();
		m_pBoneThickness->setChart(m_pBoneThicknessChartBar);
		m_pBoneThickness->setTable(m_pBoneThicknessTable);
		m_pBoneThickness->setWindow();
		QApplication::restoreOverrideCursor();
	}
}

void iABoneThicknessTool::slotPushButtonSave()
{
	QPushButton* pPushButtonSave((QPushButton*)sender());

	QFileDialog pFileDialog;
	pFileDialog.setAcceptMode(QFileDialog::AcceptSave);
	pFileDialog.setDefaultSuffix("csv");
	pFileDialog.setFileMode(QFileDialog::ExistingFile);
	pFileDialog.setNameFilter("CSV file (*.csv)");
	pFileDialog.setWindowTitle(pPushButtonSave->text());

	if (pFileDialog.exec())
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		m_pBoneThickness->save(pFileDialog.selectedFiles().first());
		QApplication::restoreOverrideCursor();
	}
}
