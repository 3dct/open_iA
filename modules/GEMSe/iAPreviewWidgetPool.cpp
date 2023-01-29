// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPreviewWidgetPool.h"

#include "iAImagePreviewWidget.h"

#include <iALog.h>

iAPreviewWidgetPool::iAPreviewWidgetPool(int maxWidgets, vtkCamera* camera, iASlicerMode slicerMode, int labelCount, iAColorTheme const * colorTheme):
	m_commonCamera(camera),
	m_slicerMode(slicerMode),
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
		LOG(lvlError, "No more slicer widgets available!\n");
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
		LOG(lvlError, QString("Current and given sliceNumber unexpectedly don't match (sliceNr=%1 != m_sliceNumber[mode]=%2\n")
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
