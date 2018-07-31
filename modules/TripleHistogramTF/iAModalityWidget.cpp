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
 
#include "iAModalityWidget.h"

#include "vtkImageData.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QString>

// Debug
#include <QDebug>

// TODO: does this belong here?
static const char *m_weightFormat = "%.10f";

// Required to load histogram
#include "iAModalityList.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "iAPreferences.h"

// Required to create slicer
#include "vtkColorTransferFunction.h"
#include "vtkCamera.h"
#include "iASlicerData.h"

iAModalityWidget::iAModalityWidget(QWidget * parent, QSharedPointer<iAModality> modality, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	QWidget *rightWidget = new QWidget(this);

	//QLabel *slicer = new QLabel(rightWidget);
	//slicer->setText("Slicer");

	vtkImageData *imageData = mdiChild->getImageData();

	// ----------------------------------------------------------------------------------------------------------
	// Initialize slicer

	QWidget *slicerWidget = new QWidget(rightWidget);
	m_slicer = new iASlicer(rightWidget, iASlicerMode::XY, slicerWidget,
		// Value of shareWidget is defaulted to 0 in the iASlicer constructor... that's why I do that here
		// TODO: do this in a better way?
		/*QGLWidget * shareWidget = */0,
		/*Qt::WindowFlags f = */f,
		/*bool decorations = */false); // Hide everything except the slice itself

	vtkColorTransferFunction* colorFunction = modality->GetTransfer()->GetColorFunction();
	m_slicerTransform = vtkTransform::New();
	m_slicer->initializeData(imageData, m_slicerTransform, colorFunction);

	// TODO: deactivate interaction with the slice (zoom, pan, etc)
	// TODO: fill widget with the sliced image

	// Initialize slicer
	// ----------------------------------------------------------------------------------------------------------
	// Initialize histogram

	if (!modality->GetHistogramData() || modality->GetHistogramData()->GetNumBin() != mdiChild->GetPreferences().HistogramBins)
	{
		modality->ComputeImageStatistics();
		modality->ComputeHistogramData(mdiChild->GetPreferences().HistogramBins);
	}

	iADiagramFctWidget* histogram = new iADiagramFctWidget(this, mdiChild);
	QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
		new	iABarGraphDrawer(modality->GetHistogramData(), QColor(70, 70, 70, 255)));
	histogram->AddPlot(histogramPlot);
	histogram->SetTransferFunctions(modality->GetTransfer()->GetColorFunction(),
		modality->GetTransfer()->GetOpacityFunction());

	histogram->updateTrf();

	// Initialize histogram
	// ----------------------------------------------------------------------------------------------------------

	m_weightLabel = new QLabel(rightWidget);
	m_weightLabel->setText("Weight");

	QVBoxLayout *rightWidgetLayout = new QVBoxLayout(rightWidget);
	rightWidgetLayout->addWidget(slicerWidget);
	rightWidgetLayout->addWidget(m_weightLabel);

	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	mainLayout->addWidget((QWidget*) histogram); // TODO: why do I need to cast a subclass into its superclass?
	mainLayout->addWidget(rightWidget);
}

iAModalityWidget::~iAModalityWidget()
{
	m_slicerTransform->Delete();

	delete m_slicer;
}

void iAModalityWidget::setWeight(double weight)
{
	qDebug() << "Modality widget setWeight(double) method called!";
	QString text;
	m_weightLabel->setText(text.sprintf(m_weightFormat, weight));
}

void iAModalityWidget::setSlicerMode(iASlicerMode slicerMode)
{
	qDebug() << "Modality widget setSlicerMode(iASlicerMode) method called!";
	m_slicer->ChangeMode(slicerMode);
	m_slicer->update();
}

void iAModalityWidget::setSliceNumber(int sliceNumber)
{
	qDebug() << "Modality widget setSliceNumber(int) method called!";
	m_slicer->setSliceNumber(sliceNumber);
	m_slicer->update();
}