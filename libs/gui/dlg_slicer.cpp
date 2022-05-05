/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iASlicerImpl.h"

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

dlg_slicer::dlg_slicer(iASlicerImpl* slicer):
	m_slicer(slicer)
{
	setupUi(this);
	QString slicePlaneName = slicerModeString(slicer->mode());
	QString sliceAxisString = axisName(mapSliceToGlobalAxis(slicer->mode(), iAAxisIndex::Z));
	QColor color(slicerColor(slicer->mode()));
	setObjectName(QString("slice%1").arg(slicePlaneName));
	setWindowTitle(QString("Slice %1").arg(slicePlaneName));
	lbTitle->setText(slicePlaneName);
	lbSlice->setText(QString("Slice # %1").arg(sliceAxisString));
	lbRotation->setText(QString("Rot %1").arg(sliceAxisString));
	sliceContainerLayout->addWidget(slicer);
	sliceContainer->setStyleSheet(QString("#sliceContainer { border: %1px solid rgb(%2, %3, %4) } ")
		.arg(BorderWidth).arg(color.red()).arg(color.green()).arg(color.blue()));
	sbSlice->setRange(-8192, 8192);
	sbSlabThickness->hide();
	lbSlabThickness->hide();
	cbSlabCompositeMode->hide();

	connect(pbSave, &QToolButton::clicked, slicer, &iASlicer::saveAsImage);
	connect(pbSaveStack, &QToolButton::clicked, slicer, &iASlicerImpl::saveImageStack);
	connect(pbMov, &QToolButton::clicked, slicer, &iASlicer::saveMovie);
	connect(pbStop, &QToolButton::clicked, slicer, &iASlicerImpl::toggleInteractorState);
	connect(dsbRotation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), slicer, &iASlicer::rotateSlice);
	connect(sbSlice, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_slicer::setSliceSpinBox);
	connect(verticalScrollBar, &QSlider::valueChanged, this, &dlg_slicer::setSliceScrollBar);
	connect(cbSlabMode, &QCheckBox::toggled, this, &dlg_slicer::setSlabMode);
	connect(sbSlabThickness, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_slicer::updateSlabThickness);
	connect(cbSlabCompositeMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_slicer::updateSlabCompositeMode);
	connect(m_slicer, &iASlicerImpl::sliceRangeChanged, this, &dlg_slicer::updateSliceControls);
}

void dlg_slicer::showBorder(bool show)
{
	int borderWidth = show ? BorderWidth : 0;
	sliceContainerLayout->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);
}

void dlg_slicer::setSliceSpinBox(int s)
{
	m_slicer->setSliceNumber(s);
}

void dlg_slicer::setSliceScrollBar(int s)
{
	m_slicer->setSliceNumber(s);
}

void dlg_slicer::setSlabMode(bool slabMode)
{
	lbSlabThickness->setVisible(slabMode);
	sbSlabThickness->setVisible(slabMode);
	cbSlabCompositeMode->setVisible(slabMode);
	updateSlabThickness(slabMode == true ? sbSlabThickness->value() : 0);
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