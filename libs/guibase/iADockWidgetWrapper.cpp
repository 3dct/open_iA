// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADockWidgetWrapper.h"

iADockWidgetWrapper::iADockWidgetWrapper(QWidget* widget, QString const & windowTitle, QString const & objectName):
	m_titleBar(new QWidget())
{
	setWindowTitle(windowTitle);
	setFeatures(DockWidgetVerticalTitleBar | DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable);
	setWidget(widget);
	setObjectName(objectName);
}

void iADockWidgetWrapper::toggleTitleBar()
{
	QWidget* titleBar = titleBarWidget();
	if (titleBar == m_titleBar)
	{
		setTitleBarWidget(nullptr);
	}
	else
	{
		setTitleBarWidget(m_titleBar);
	}
}

bool iADockWidgetWrapper::isTitleBarVisible() const
{
	return titleBarWidget() != nullptr;
}
