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
#include "RightBorderLayout.h"

#include "vtkImageData.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QString>

// Debug
#include <QDebug>

// Required for the histogram
#include "iAModalityList.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "iAPreferences.h"
#include "dlg_transfer.h"

// Required to create slicer
#include "vtkColorTransferFunction.h"
#include "vtkCamera.h"
#include "iASlicerData.h"

// TODO: does this belong here?
static const char *m_weightFormat = "%.10f";

iAModalityWidget::iAModalityWidget(QWidget * parent, QSharedPointer<iAModality> modality, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	QWidget *rightWidget = new QWidget(this);

	//QLabel *slicer = new QLabel(rightWidget);
	//slicer->setText("Slicer");

	vtkImageData *imageData = modality->GetImage().GetPointer();

	// ----------------------------------------------------------------------------------------------------------
	// Initialize slicer

	m_slicerWidget = new iASimpleSlicerWidget(rightWidget, modality, imageData, f);

	// Initialize slicer
	// ----------------------------------------------------------------------------------------------------------
	// Initialize histogram

	if (!modality->GetHistogramData() || modality->GetHistogramData()->GetNumBin() != mdiChild->GetPreferences().HistogramBins)
	{
		modality->ComputeImageStatistics();
		modality->ComputeHistogramData(mdiChild->GetPreferences().HistogramBins);
	}

	m_histogram = new iADiagramFctWidget(this, mdiChild);
	QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
		new	iABarGraphDrawer(modality->GetHistogramData(), QColor(70, 70, 70, 255)));
	m_histogram->AddPlot(histogramPlot);
	m_histogram->SetTransferFunctions(modality->GetTransfer()->GetColorFunction(),
		modality->GetTransfer()->GetOpacityFunction());

	m_histogram->updateTrf();

	//connect(histogram, SIGNAL(updateViews()), this, SLOT(updateViews()));
	//connect(histogram, SIGNAL(pointSelected()), this, SIGNAL(pointSelected()));
	//connect(histogram, SIGNAL(noPointSelected()), this, SIGNAL(noPointSelected()));
	//connect(histogram, SIGNAL(endPointSelected()), this, SIGNAL(endPointSelected()));
	//connect(histogram, SIGNAL(active()), this, SIGNAL(active()));
	//connect(histogram, SIGNAL(autoUpdateChanged(bool)), this, SIGNAL(autoUpdateChanged(bool)));
	//connect((dlg_transfer*)(m_histogram->getFunctions()[0]), SIGNAL(Changed()), mdiChild, SLOT(ModalityTFChanged()));
	connect((dlg_transfer*)(m_histogram->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction()));

	// Initialize histogram
	// ----------------------------------------------------------------------------------------------------------

	m_weightLabel = new QLabel(rightWidget);
	//m_weightLabel->setText("Weight");

	// TODO: use a better name
	m_modalityLabel = new QLabel(modality->GetName());

	m_rightWidgetLayout = new QVBoxLayout(rightWidget);
	m_rightWidgetLayout->addWidget(m_slicerWidget);
	m_rightWidgetLayout->addWidget(m_modalityLabel);
	m_rightWidgetLayout->addWidget(m_weightLabel);
	//RightBorderLayout *rightWidgetLayout = new RightBorderLayout(rightWidget, RightBorderLayout::Top);
	//rightWidgetLayout->setBorderWidget(new SquareBorderWidget(m_slicerWidget));
	//rightWidgetLayout->setCenterWidget(m_weightLabel);

	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->addWidget(m_histogram); // TODO: why do I need to cast a subclass into its superclass?
	m_mainLayout->addWidget(rightWidget);
	//RightBorderLayout *mainLayout = new RightBorderLayout(this, RightBorderLayout::Right);
	//mainLayout->setCenterWidget(histogram);
	//mainLayout->setBorderWidget(new SquareBorderWidget(rightWidget));
}

iAModalityWidget::~iAModalityWidget()
{
	delete m_slicerWidget;
}

void iAModalityWidget::setWeight(double weight)
{
	QString text;
	m_weightLabel->setText(text.sprintf(m_weightFormat, weight));
}

void iAModalityWidget::setSlicerMode(iASlicerMode slicerMode, int dimensionLength)
{
	m_slicerWidget->changeMode(slicerMode, dimensionLength);
}

void iAModalityWidget::setSliceNumber(int sliceNumber)
{
	m_slicerWidget->setSliceNumber(sliceNumber);
}

void iAModalityWidget::resizeEvent(QResizeEvent* event)
{
	// TODO: create own layout instead of doing this?
	int hText = m_weightLabel->minimumSize().height();
	int hSlice = event->size().height() - hText;
	m_mainLayout->setStretch(0, event->size().width() - hSlice);
	m_mainLayout->setStretch(1,							hSlice);

	int h = m_weightLabel->sizeHint().height();
	m_rightWidgetLayout->setStretch(0, hSlice);
	m_rightWidgetLayout->setStretch(1, hText);
}

iATransferFunction* iAModalityWidget::getTransferFunction()
{
	// TODO: can this cast really work? handle it well instead of returning 0
	if (m_histogram->getSelectedFunction()->getType() != dlg_function::TRANSFER)
	{
		return 0;
	}

	dlg_transfer* transfer = (dlg_transfer*) m_histogram->getSelectedFunction();
	return new iASimpleTransferFunction(transfer->GetColorFunction(), transfer->GetOpacityFunction());
}

void iAModalityWidget::updateTransferFunction()
{
	m_slicerWidget->update();
	emit modalityTfChanged();
}

void iAModalityWidget::setModalityLabel(QString label)
{
	m_modalityLabel->setText(label);
}