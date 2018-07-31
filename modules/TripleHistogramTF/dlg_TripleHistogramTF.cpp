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
 
#include "dlg_TripleHistogramTF.h"

// TODO: why tf do I need this?
#include "iAModalityList.h"

// Debug
#include "qdebug.h"

dlg_TripleHistogramTF::dlg_TripleHistogramTF(MdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	TripleHistogramTFConnector(mdiChild, f), m_mdiChild(mdiChild)
{
	QWidget *optionsContainer = new QWidget();
	optionsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m_slicerModeComboBox = new QComboBox(optionsContainer);
	m_slicerModeComboBox->addItem("YZ", QVariant(iASlicerMode::YZ));
	m_slicerModeComboBox->addItem("XY", QVariant(iASlicerMode::XY));
	m_slicerModeComboBox->addItem("XZ", QVariant(iASlicerMode::XZ));

	m_sliceSlider = new QSlider(Qt::Horizontal, optionsContainer);
	m_sliceSlider->setMinimum(0);

	QHBoxLayout *optionsContainerLayout = new QHBoxLayout(optionsContainer);
	optionsContainerLayout->addWidget(m_slicerModeComboBox);
	optionsContainerLayout->addWidget(m_sliceSlider);

	QWidget *histogramStackContainer = new QWidget();
	QVBoxLayout *histogramStackContainerLayout = new QVBoxLayout(histogramStackContainer);

	// TODO: load 3 DIFFERENT modalities
	if (m_mdiChild->GetModalities()->size() > 0)
	{
		iAModalityWidget *modalities[3];
		int i = 0;
		for (int j = 0; j < 3/*mdiChild->GetModalities()->size()*/; ++j)
		{
			modalities[j] = new iAModalityWidget(histogramStackContainer, mdiChild->GetModality(i), mdiChild);
			histogramStackContainerLayout->addWidget(modalities[j]);
		}
		modality1 = modalities[0];
		modality2 = modalities[1];
		modality3 = modalities[2];
	}
	else {
		modality1 = 0;
		modality2 = 0;
		modality3 = 0;
	}

	triangle = new iABarycentricTriangleWidget(dockWidgetContents);
	
	connect(triangle, SIGNAL(weightChanged(BCoord)), this, SLOT(setWeight(BCoord)));
	connect(m_slicerModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSlicerMode()));
	connect(m_sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliceNumber(int)));

	QWidget *leftWidget = new QWidget();
	//leftWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	QVBoxLayout *leftWidgetLayout = new QVBoxLayout(leftWidget);
	leftWidgetLayout->addWidget(optionsContainer);
	leftWidgetLayout->addWidget(histogramStackContainer);

	//QLayout *mainLayout = dockWidgetContents->layout();
	//mainLayout->addWidget(leftWidget);
	//mainLayout->addWidget(triangle);
	mainLayout->addWidget(leftWidget);
	mainLayout->addWidget(triangle);

	// Initialize
	updateSlicerMode();
}

dlg_TripleHistogramTF::~dlg_TripleHistogramTF()
{}

void dlg_TripleHistogramTF::resizeEvent(QResizeEvent* event)
{
	// TODO: create own layout instead of doing this
	int w = triangle->getWidthForHeight(event->size().height());
	mainLayout->setStretch(0, event->size().width() - w);
	mainLayout->setStretch(1,						  w);
}

void dlg_TripleHistogramTF::setWeight(BCoord bCoord)
{
	qDebug() << "setWeight(BCoord) called!";
	modality1->setWeight(bCoord.getAlpha());
	modality2->setWeight(bCoord.getBeta());
	modality3->setWeight(bCoord.getGamma());
}

void dlg_TripleHistogramTF::updateSlicerMode()
{
	qDebug() << "updateSlicerMode() called!";
	setSlicerMode((iASlicerMode) m_slicerModeComboBox->currentData().toInt());
}

void dlg_TripleHistogramTF::setSlicerMode(iASlicerMode slicerMode)
{
	int dimensionIndex;

	switch (slicerMode)
	{
	case iASlicerMode::YZ:
		dimensionIndex = 0; // X length is in position 0 in dimensions array
		break;
	case iASlicerMode::XZ:
		dimensionIndex = 1; // Y length is in position 1 in dimensions array
		break;
	case iASlicerMode::XY:
		dimensionIndex = 2; // Z length is in position 2 in dimensions array
		break;
	default:
		// TODO?
		return;
	}

	int dimensionLength = m_mdiChild->getImageData()->GetDimensions()[dimensionIndex];
	m_sliceSlider->setMaximum(dimensionLength - 1);

	modality1->setSlicerMode(slicerMode);
	modality2->setSlicerMode(slicerMode);
	modality3->setSlicerMode(slicerMode);
}

void dlg_TripleHistogramTF::setSliceNumber(int sliceNumber)
{
	qDebug() << "setSliceNumber(int) called!";
	modality1->setSliceNumber(sliceNumber);
	modality2->setSliceNumber(sliceNumber);
	modality3->setSliceNumber(sliceNumber);
}