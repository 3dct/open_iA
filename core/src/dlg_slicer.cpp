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

dlg_slicer::dlg_slicer(iASlicerMode mode, iASlicer* slicer)
{
	setupUi(this);
	QString slicePlaneName = getSlicerModeString(mode);
	QString sliceAxis = getSliceAxis(mode);
	QColor color(slicerColor(mode));
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
}

void dlg_slicer::showBorder(bool show)
{
	int borderWidth = show ? BorderWidth : 0;
	sliceContainerLayout->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);
}
