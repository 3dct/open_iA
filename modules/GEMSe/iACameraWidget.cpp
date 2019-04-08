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
#include "iACameraWidget.h"

#include "iAImagePreviewWidget.h"
#include "iAGEMSeConstants.h"
#include "iAQtCaptionWidget.h"

#include <iAColorTheme.h>
#include <iAConsole.h>

#include <vtkImageData.h>

#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>

namespace
{
	const iASlicerMode InitialSlicerMode = iASlicerMode::XY;

	QPoint GridSlicerMap[3] {
		QPoint(1, 2), // YZ
		QPoint(1, 1), // XY
		QPoint(0, 1)  // XZ
	};
}


iACameraWidget::iACameraWidget(QWidget* parent, vtkSmartPointer<vtkImageData> originalData, int labelCount, CameraLayout layout):
	QWidget(parent),
	m_slicerMode(InitialSlicerMode)
{
	QWidget* miniSlicerContainer = new QWidget;
	QGridLayout * gridLay = 0;
	gridLay = new QGridLayout;
	gridLay->setSpacing(CameraSpacing);
	QLabel * zLabel1 = new QLabel("z");
	QFont f(zLabel1->font());
	f.setPointSize(FontSize);
	zLabel1->setFont(f);
	QLabel * yLabel  = new QLabel("y");
	yLabel->setFont(f);
	QLabel * xLabel  = new QLabel("x");
	xLabel->setFont(f);
	QLabel * zLabel2 = new QLabel("z");
	zLabel2->setFont(f);
	m_sliceLabel = new QLabel("");
	m_sliceLabel->setFont(f);

	gridLay->addWidget(zLabel1, 0, 0, Qt::AlignCenter);
	gridLay->addWidget(yLabel, 1, 0, Qt::AlignCenter);
	gridLay->addWidget(xLabel, 2, 1, Qt::AlignCenter);
	gridLay->addWidget(zLabel2, 2, 2, Qt::AlignCenter);
	gridLay->addWidget(m_sliceLabel, 0, 2, Qt::AlignCenter);

	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		QString caption(slicerModeString(i));
		m_sliceViews[i] = new iAImagePreviewWidget(QString("CameraView")+caption,
			0, false, 0, static_cast<iASlicerMode>(i), labelCount);
		m_sliceViews[i]->setImage(originalData, false, false);
		m_sliceViews[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		/*
		// why should that be necessary?
		if (i==0)
		{
			m_sliceViews[i]->GetSlicer()->rotateSlice(-90);
		}
		*/

		gridLay->addWidget(m_sliceViews[i], GridSlicerMap[i].x(), GridSlicerMap[i].y());
		connect(m_sliceViews[i], SIGNAL(Clicked()), this, SLOT( MiniSlicerClicked() ));
		connect(m_sliceViews[i], SIGNAL(Updated()), this, SLOT( MiniSlicerUpdated() ));
	}
	miniSlicerContainer->setLayout(gridLay);

	m_sliceScrollBar = new QScrollBar(Qt::Vertical);

	m_commonCamera = m_sliceViews[static_cast<int>(InitialSlicerMode)]->GetCamera();
	QHBoxLayout* mainLay = new QHBoxLayout;
	mainLay->setSpacing(CameraSpacing);
	mainLay->setContentsMargins(0,0,0,0);
	mainLay->addWidget(m_sliceScrollBar, 1);
	mainLay->addWidget(miniSlicerContainer, 10);
	setLayout(mainLay);
	UpdateScrollBar(m_sliceViews[static_cast<int>(InitialSlicerMode)]->GetSliceNumber());

	connect(m_sliceScrollBar, SIGNAL(valueChanged(int)), this, SLOT(ScrollBarChanged(int)));
}

void iACameraWidget::UpdateScrollBar(int sliceNumber)
{
	int extent[6];
	m_sliceViews[static_cast<int>(InitialSlicerMode)]->image()->GetExtent(extent);
	int minIdx = (m_slicerMode == iASlicerMode::XY) ? 4 : (
		(m_slicerMode == iASlicerMode::YZ) ? 0 : 2
	);
	int maxIdx = minIdx + 1;
	m_sliceScrollBar->setRange(0, extent[maxIdx]-extent[minIdx]);
	m_sliceScrollBar->setValue(sliceNumber);
	UpdateSliceLabel(sliceNumber);
}

void iACameraWidget::MiniSlicerClicked()
{
	iAImagePreviewWidget* miniSlicer = dynamic_cast<iAImagePreviewWidget*>(sender());
	assert(miniSlicer);
	if (!miniSlicer)
	{
		return;
	}
	m_commonCamera = miniSlicer->GetCamera();
	m_slicerMode = miniSlicer->GetSlicerMode();
	QSignalBlocker blockScrollSignal(m_sliceScrollBar);
	UpdateScrollBar(miniSlicer->GetSliceNumber());
	emit ModeChanged(miniSlicer->GetSlicerMode(), miniSlicer->GetSliceNumber());
}

vtkCamera* iACameraWidget::GetCommonCamera()
{
	return m_commonCamera;
}

void iACameraWidget::ScrollBarChanged(int value)
{
	m_sliceViews[m_slicerMode]->SetSliceNumber(value);
	UpdateSliceLabel(value);
	emit SliceChanged(value);
}

void iACameraWidget::UpdateSliceLabel(int sliceNumber)
{
	m_sliceLabel->setText(QString("Selected Axis: %1\nSlice: %2")
		.arg(slicerModeString(m_slicerMode))
		.arg(sliceNumber));
}

void iACameraWidget::UpdateView()
{
	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		m_sliceViews[i]->UpdateView();
	}
}

void iACameraWidget::MiniSlicerUpdated()
{
	iAImagePreviewWidget* miniSlicer = dynamic_cast<iAImagePreviewWidget*>(sender());
	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		if (m_sliceViews[i] != miniSlicer)
		{
			m_sliceViews[i]->UpdateView();
		}
	}
	emit ViewUpdated();
}


void iACameraWidget::ShowImage(vtkSmartPointer<vtkImageData> imgData)
{
	if (!imgData)
	{
		DEBUG_LOG("CameraWidget: image data is NULL!\n");
		return;
	}
	
	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		m_sliceViews[i]->setImage(imgData, false, false);
	}
}