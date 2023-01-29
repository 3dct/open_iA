// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>

#include <vtkSmartPointer.h>

#include <QWidget>

class vtkCamera;
class vtkImageData;
class iAImagePreviewWidget;

class QLabel;
class QScrollBar;

class iACameraWidget: public QWidget
{
	Q_OBJECT
public:
	enum CameraLayout
	{
		ListLayout,
		GridLayout
	};
	iACameraWidget(QWidget* parent, vtkSmartPointer<vtkImageData>, int labelCount, CameraLayout layout);
	vtkCamera* commonCamera();
	void updateView();
	void showImage(vtkSmartPointer<vtkImageData> imgData);

signals:
	void ModeChanged(iASlicerMode newMode, int sliceNr);
	void SliceChanged(int sliceNr);
	void ViewUpdated();

private:
	void updateScrollBar(int sliceNumber);
	void updateSliceLabel(int sliceNumber);

	static const int SLICE_VIEW_COUNT = 3;
	iAImagePreviewWidget* m_mainView;
	iAImagePreviewWidget* m_sliceViews[SLICE_VIEW_COUNT];
	vtkCamera*            m_commonCamera;
	QScrollBar*           m_sliceScrollBar;
	iASlicerMode          m_slicerMode;
	QLabel*               m_sliceLabel;

private slots:
	void MiniSlicerClicked();
	void MiniSlicerUpdated();
	void ScrollBarChanged(int);
};
