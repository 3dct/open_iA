// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACameraWidget.h"

#include "iAImagePreviewWidget.h"
#include "iAGEMSeConstants.h"
#include "iAQtCaptionWidget.h"

#include <iAColorTheme.h>
#include <iALog.h>

#include <vtkImageData.h>
#include <vtkCamera.h>

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


iACameraWidget::iACameraWidget(QWidget* parent, vtkSmartPointer<vtkImageData> originalData, int labelCount, CameraLayout /*layout*/):
	QWidget(parent),
	m_slicerMode(InitialSlicerMode)
{
	QWidget* miniSlicerContainer = new QWidget;
	QGridLayout * gridLay = new QGridLayout;
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
			nullptr, false, nullptr, static_cast<iASlicerMode>(i), labelCount);
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
		connect(m_sliceViews[i], &iAImagePreviewWidget::clicked, this, &iACameraWidget::MiniSlicerClicked);
		connect(m_sliceViews[i], &iAImagePreviewWidget::updated, this, &iACameraWidget::MiniSlicerUpdated);
		m_sliceViews[i]->resetCamera();
	}
	miniSlicerContainer->setLayout(gridLay);

	m_sliceScrollBar = new QScrollBar(Qt::Vertical);

	m_commonCamera = m_sliceViews[static_cast<int>(InitialSlicerMode)]->camera();
	QHBoxLayout* mainLay = new QHBoxLayout;
	mainLay->setSpacing(CameraSpacing);
	mainLay->setContentsMargins(0,0,0,0);
	mainLay->addWidget(m_sliceScrollBar, 1);
	mainLay->addWidget(miniSlicerContainer, 10);
	setLayout(mainLay);
	updateScrollBar(m_sliceViews[static_cast<int>(InitialSlicerMode)]->sliceNumber());

	connect(m_sliceScrollBar, &QScrollBar::valueChanged, this, &iACameraWidget::ScrollBarChanged);
}

void iACameraWidget::updateScrollBar(int sliceNumber)
{
	int extent[6];
	m_sliceViews[static_cast<int>(InitialSlicerMode)]->image()->GetExtent(extent);
	int minIdx = (m_slicerMode == iASlicerMode::XY) ? 4 : (
		(m_slicerMode == iASlicerMode::YZ) ? 0 : 2
	);
	int maxIdx = minIdx + 1;
	m_sliceScrollBar->setRange(0, extent[maxIdx]-extent[minIdx]);
	m_sliceScrollBar->setValue(sliceNumber);
	updateSliceLabel(sliceNumber);
}

void iACameraWidget::MiniSlicerClicked()
{
	iAImagePreviewWidget* miniSlicer = dynamic_cast<iAImagePreviewWidget*>(sender());
	assert(miniSlicer);
	if (!miniSlicer)
	{
		return;
	}
	m_commonCamera = miniSlicer->camera();
	m_slicerMode = miniSlicer->slicerMode();
	QSignalBlocker blockScrollSignal(m_sliceScrollBar);
	updateScrollBar(miniSlicer->sliceNumber());
	emit ModeChanged(miniSlicer->slicerMode(), miniSlicer->sliceNumber());
}

vtkCamera* iACameraWidget::commonCamera()
{
	return m_commonCamera;
}

void iACameraWidget::ScrollBarChanged(int value)
{
	m_sliceViews[m_slicerMode]->setSliceNumber(value);
	updateSliceLabel(value);
	emit SliceChanged(value);
}

void iACameraWidget::updateSliceLabel(int sliceNumber)
{
	m_sliceLabel->setText(QString("Selected Axis: %1\nSlice: %2")
		.arg(slicerModeString(m_slicerMode))
		.arg(sliceNumber));
}

void iACameraWidget::updateView()
{
	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		m_sliceViews[i]->updateView();
	}
}

void iACameraWidget::MiniSlicerUpdated()
{
	iAImagePreviewWidget* miniSlicer = dynamic_cast<iAImagePreviewWidget*>(sender());
	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		if (m_sliceViews[i] != miniSlicer)
		{
			m_sliceViews[i]->updateView();
		}
	}
	emit ViewUpdated();
}


void iACameraWidget::showImage(vtkSmartPointer<vtkImageData> imgData)
{
	if (!imgData)
	{
		LOG(lvlError, "CameraWidget: image data is nullptr!\n");
		return;
	}

	for (int i=0; i<SLICE_VIEW_COUNT; ++i)
	{
		m_sliceViews[i]->setImage(imgData, false, false);
	}
}
