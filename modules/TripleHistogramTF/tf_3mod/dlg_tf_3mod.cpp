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

#include "dlg_tf_3mod.h"

#include "iATripleModalityWidget.h"
#include "iABarycentricContextRenderer.h"
#include "iAHistogramStack.h"
#include "iAHistogramTriangle.h"

#include <dlg_modalities.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAModalityTransfer.h>
#include <iARenderer.h>
#include <iASlicerData.h>
#include <iAVolumeRenderer.h>
#include <mdichild.h>

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

const static QString DEFAULT_LABELS[3] = { "A", "B", "C" };

dlg_tf_3mod::dlg_tf_3mod(MdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */)
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

	m_tripleModalityWidget = new iATripleModalityWidget(dockWidgetContents, mdiChild);
	layout->addWidget(m_tripleModalityWidget);
}