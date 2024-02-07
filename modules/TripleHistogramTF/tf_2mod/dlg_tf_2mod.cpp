// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dlg_tf_2mod.h"

#include "iABimodalWidget.h"

#include <iAMdiChild.h>

#include <QHBoxLayout>

dlg_tf_2mod::dlg_tf_2mod(iAMdiChild* mdiChild)
	:
	QDockWidget("Double Histogram Transfer Function", mdiChild),
	m_mdiChild(mdiChild)
{
	// Initialize dock widget
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);

	QWidget *dockWidgetContents = new QWidget();
	//QSplitter *dockWidgetContents = new QSplitter(Qt::Horizontal);
	setWidget(dockWidgetContents);
	dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
	QHBoxLayout *layout = new QHBoxLayout(dockWidgetContents);

	m_bimodalWidget = new iABimodalWidget(mdiChild);
	layout->addWidget(m_bimodalWidget);
}
