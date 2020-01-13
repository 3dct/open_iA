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
#include "iAPreviewWidgetPool.h"

#include "iAImagePreviewWidget.h"

#include <iAConsole.h>

iAPreviewWidgetPool::iAPreviewWidgetPool(int maxWidgets, vtkCamera* camera, iASlicerMode slicerMode, int labelCount, iAColorTheme const * colorTheme)
	: m_slicerMode(slicerMode),
	m_commonCamera(camera),
	m_maxWidgets(maxWidgets),
	m_labelCount(labelCount),
	m_colorTheme(colorTheme)
{
	for (int i=0; i<maxWidgets; ++i)
	{
		iAImagePreviewWidget* newW = new iAImagePreviewWidget(QString("SlicerView")+QString::number(i),
				0, true, m_commonCamera, m_slicerMode, m_labelCount, true);
		newW->setColorTheme(m_colorTheme);
		newW->hide();
		m_pool.push_back(newW);
	}
	std::fill(m_sliceNumber, m_sliceNumber+SlicerCount, iAImagePreviewWidget::SliceNumberNotSet);
}

iAImagePreviewWidget* iAPreviewWidgetPool::getWidget(QWidget* parent, bool /*magicLens*/)
{
	if (m_pool.size() == 0)
	{
#if _DEBUG
		DEBUG_LOG("No more slicer widgets available!\n");
#endif
		return 0;
	}
	iAImagePreviewWidget* result = m_pool[m_pool.size()-1];
	m_pool.remove(m_pool.size()-1);
	result->setParent(parent);
	result->show();
	result->setSlicerMode(m_slicerMode, m_sliceNumber[m_slicerMode], m_commonCamera);
	if (m_sliceNumber[m_slicerMode] != iAImagePreviewWidget::SliceNumberNotSet)
	{
		result->setSliceNumber(m_sliceNumber[m_slicerMode]);
	}
	m_visible.push_back(result);
	return result;
}

void iAPreviewWidgetPool::returnWidget(iAImagePreviewWidget* widget)
{
	widget->hide();
	widget->setParent(0);
	int idx = m_visible.lastIndexOf(widget);
	assert( idx != -1 );
	if (idx != -1)
	{
		m_visible.remove(idx);
	}
	m_pool.push_back(widget);
}

void iAPreviewWidgetPool::setSlicerMode(iASlicerMode mode, int sliceNr, vtkCamera* camera)
{
	m_slicerMode = mode;
	if (m_sliceNumber[m_slicerMode] != sliceNr && m_sliceNumber[m_slicerMode] != iAImagePreviewWidget::SliceNumberNotSet)
	{
		DEBUG_LOG(QString("Current and given sliceNumber unexpectedly don't match (sliceNr=%1 != m_sliceNumber[mode]=%2\n")
			.arg(sliceNr)
			.arg(m_sliceNumber[m_slicerMode]));
	}
	m_sliceNumber[m_slicerMode] = sliceNr;
	m_commonCamera = camera;
	for (iAImagePreviewWidget* widget: m_visible)
	{
		widget->setSlicerMode(mode, sliceNr, camera);
	}
}

int iAPreviewWidgetPool::capacity()
{
	return m_pool.size();
}

void iAPreviewWidgetPool::setSliceNumber(int sliceNumber)
{
	m_sliceNumber[m_slicerMode] = sliceNumber;
	for (iAImagePreviewWidget* widget: m_visible)
	{
		widget->setSliceNumber(sliceNumber);
	}
}

void iAPreviewWidgetPool::updateViews()
{
	for (iAImagePreviewWidget* nodeWidget: m_visible)
	{
		if (nodeWidget->isVisible())
		{
			nodeWidget->updateView();
		}
	}
}

void iAPreviewWidgetPool::setColorTheme(iAColorTheme const * colorTheme)
{
	m_colorTheme = colorTheme;
	for (iAImagePreviewWidget* nodeWidget: m_visible)
	{
		nodeWidget->setColorTheme(colorTheme);
	}
	for (iAImagePreviewWidget* nodeWidget: m_pool)
	{
		nodeWidget->setColorTheme(colorTheme);
	}
}