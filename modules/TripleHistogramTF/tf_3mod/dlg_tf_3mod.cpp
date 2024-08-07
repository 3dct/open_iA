// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dlg_tf_3mod.h"

#include "iATripleModalityWidget.h"

#include <iAMdiChild.h>

#include <QHBoxLayout>

dlg_tf_3mod::dlg_tf_3mod(iAMdiChild * mdiChild, Qt::WindowFlags f)
	:
	//TripleHistogramTFConnector(mdiChild, f), m_mdiChild(mdiChild)
	QDockWidget("Triple Histogram Transfer Function", mdiChild, f)
{

	// Initialize dock widget
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);

	//resize(779, 501);

	QWidget *dockWidgetContents = new QWidget();
	//QSplitter *dockWidgetContents = new QSplitter(Qt::Horizontal);
	setWidget(dockWidgetContents);
	dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
	QHBoxLayout *layout = new QHBoxLayout(dockWidgetContents);

	m_tripleModalityWidget = new iATripleModalityWidget(mdiChild);
	layout->addWidget(m_tripleModalityWidget);
}
