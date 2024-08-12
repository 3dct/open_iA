// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADockWidgetWrapper.h"
#include "iAQDockTitleWidget.h"

iADockWidgetWrapper::iADockWidgetWrapper(QWidget* widget, QString const & windowTitle, QString const & objectName, QString const& infoLink):
	m_titleWidget(nullptr)
{
	setWindowTitle(windowTitle);
	setFeatures(DockWidgetVerticalTitleBar | DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable);
	setWidget(widget);
	setObjectName(objectName);
	if (!infoLink.isEmpty())
	{
		m_titleWidget = new iAQDockTitleWidget(this, infoLink);
		setTitleBarWidget(m_titleWidget);
	}
}

void iADockWidgetWrapper::toggleTitleBar()
{
	setTitleBarWidget( (titleBarWidget() == m_titleWidget) ? new QWidget() : m_titleWidget);
}

bool iADockWidgetWrapper::isTitleBarVisible() const
{
	return titleBarWidget() == m_titleWidget;
}
