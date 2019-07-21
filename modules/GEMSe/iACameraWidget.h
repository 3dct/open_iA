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
