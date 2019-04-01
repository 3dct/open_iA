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
#include "dlg_slicer.h"

#include "iASlicer.h"

const int dlg_slicer::BorderWidth = 3;

QColor dlg_slicer::slicerColor(iASlicerMode mode)
{
	switch (mode)
	{
		case iASlicerMode::YZ: return QColor(255,   0,   0);
		case iASlicerMode::XZ: return QColor(  0, 255,   0);
		case iASlicerMode::XY: return QColor(  0,   0, 255);
		default              : return QColor(  0,   0,   0);
	}
}

dlg_slicer::dlg_slicer(iASlicer* slicer):
	m_slicer(slicer)
{
	setupUi(this);
	QString slicePlaneName = getSlicerModeString(slicer->mode());
	QString sliceAxis = getSliceAxis(slicer->mode());
	QColor color(slicerColor(slicer->mode()));
	setObjectName(QString("slice%1").arg(slicePlaneName));
	setWindowTitle(QString("Slice %1").arg(slicePlaneName));
	lbTitle->setText(slicePlaneName);
	lbSlice->setText(QString("Slice # %1").arg(sliceAxis));
	lbRotation->setText(QString("Rot %1").arg(sliceAxis));
	sliceContainerLayout->addWidget(slicer);
	sliceContainer->setStyleSheet(QString("#sliceWidget { border: %1px solid rgb(%2, %3, %4) } ")
		.arg(BorderWidth).arg(color.red()).arg(color.green()).arg(color.blue()));
	sbSlice->setRange(-8192, 8192);
	sbSlabThickness->hide();
	lbSlabThickness->hide();
	cbSlabCompositeMode->hide();
	
	connect(pbSave, &QToolButton::clicked, slicer, &iASlicer::saveAsImage);
	connect(pbSaveStack, &QToolButton::clicked, slicer, &iASlicer::saveImageStack);
	connect(pbMov, &QToolButton::clicked, slicer, &iASlicer::saveMovie);
	connect(pbStop, &QToolButton::clicked, slicer, &iASlicer::toggleInteractorState);
	connect(dsbRotation, SIGNAL(valueChanged(double)), slicer, SLOT(rotateSlice(double)));
	connect(sbSlice, SIGNAL(valueChanged(int)), this, SLOT(setSliceSpinBox(int)));
	connect(verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setSliceScrollBar(int)));
	connect(cbSlabMode, &QCheckBox::toggled, this, &dlg_slicer::setSlabMode);
	connect(sbSlabThickness, SIGNAL(valueChanged(int)), this, SLOT(updateSlabThickness(int)));
	connect(cbSlabCompositeMode, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSlabCompositeMode(int)));
	connect(m_slicer, &iASlicer::firstChannelAdded, this, &dlg_slicer::updateSliceControls);
}

void dlg_slicer::showBorder(bool show)
{
	int borderWidth = show ? BorderWidth : 0;
	sliceContainerLayout->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);
}

void dlg_slicer::setSliceSpinBox(int s)
{
	QSignalBlocker block(verticalScrollBar);
	verticalScrollBar->setValue(s);
	m_slicer->setSliceNumber(s);
}

void dlg_slicer::setSliceScrollBar(int s)
{
	QSignalBlocker block(sbSlice);
	sbSlice->setValue(s);
	m_slicer->setSliceNumber(s);
}

void dlg_slicer::setSlabMode(bool slabMode)
{
	lbSlabThickness->setVisible(slabMode);
	sbSlabThickness->setVisible(slabMode);
	cbSlabCompositeMode->setVisible(slabMode);
	slabMode == true ?
		updateSlabThickness(sbSlabThickness->value()) :
		updateSlabThickness(0);
}

void dlg_slicer::updateSlabThickness(int thickness)
{
	m_slicer->setSlabThickness(thickness);
}

void dlg_slicer::updateSlabCompositeMode(int mode)
{
	m_slicer->setSlabCompositeMode(mode);
}

void dlg_slicer::updateSliceControls(int minIdx, int maxIdx)
{
	int val = (maxIdx - minIdx) / 2 + minIdx;
	sbSlice->setRange(minIdx, maxIdx);
	verticalScrollBar->setRange(minIdx, maxIdx);
	sbSlice->setValue(val);  // updates the slicer as well via sbSlice's valueChanged signal
	QSignalBlocker block(verticalScrollBar);
	verticalScrollBar->setValue(val);
}