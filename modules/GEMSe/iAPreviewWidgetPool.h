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
#pragma once

#include "iASlicerMode.h"

#include <QVector>
#include <QWidget>

class iAColorTheme;
class iAImagePreviewWidget;

class vtkCamera;

class iAPreviewWidgetPool
{
public:
	//! create a pool with the given amount of widgets
	iAPreviewWidgetPool(int maxWidgets, vtkCamera* camera, iASlicerMode slicerMode, int labelCount, iAColorTheme const * colorTheme);
	//! retrieve a widget from the pool
	iAImagePreviewWidget* GetWidget(QWidget* parent, bool magicLens=false);
	//! return widget to the pool (because it is not used anymore)
	//! hides the widget
	void ReturnWidget(iAImagePreviewWidget*);
	//! set slicer mode for all widgets
	void SetSlicerMode(iASlicerMode mode, int sliceNr, vtkCamera* camera);
	//! change the shown slice number
	void SetSliceNumber(int sliceNr);
	//! update all views
	void UpdateViews();
	//! retrieve the number of currently available widgets
	int Capacity();
	void SetColorTheme(iAColorTheme const * colorTheme);
private:
	QVector<iAImagePreviewWidget*> m_pool;
	QVector<iAImagePreviewWidget*> m_visible;
	vtkCamera*   m_commonCamera;
	iASlicerMode m_slicerMode;
	int m_sliceNumber[SlicerModeCount];
	int m_maxWidgets;
	int m_labelCount;
	iAColorTheme const * m_colorTheme;
};
