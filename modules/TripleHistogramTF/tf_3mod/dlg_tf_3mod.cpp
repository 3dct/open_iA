// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dlg_tf_3mod.h"

#include "iATripleModalityWidget.h"
#include "iABarycentricContextRenderer.h"
#include "iAHistogramStack.h"
#include "iAHistogramTriangle.h"

#include <iARenderer.h>
#include <iAVolumeRenderer.h>
#include <iAMdiChild.h>

#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageActor.h>

#include <QLabel>
#include <QSplitter>
#include <QStackedLayout>
// Debug
#include <QDebug>


dlg_tf_3mod::dlg_tf_3mod(iAMdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */)
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
