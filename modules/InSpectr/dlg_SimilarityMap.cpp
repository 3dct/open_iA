// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_SimilarityMap.h"

#include "dlg_InSpectr.h"

#include <iAMdiChild.h>

#include <QFileDialog>

dlg_SimilarityMap::dlg_SimilarityMap( QWidget *parentWidget)
: dlg_SimilarityMapContainer( parentWidget ),
  m_similarityMapWidget( new iASimilarityMapWidget( parentWidget ) ),
  m_similarityWidgetGridLayout( new QGridLayout( widget_Container ) )
{
	m_similarityWidgetGridLayout->setContentsMargins( 0, 0, 0, 0 );
	m_similarityWidgetGridLayout->setObjectName( "SimilarityWigetGridLayout" );
	m_similarityWidgetGridLayout->addWidget( m_similarityMapWidget.data() );
	connectSignalsToSlots();
	windowingChanged();
}

void dlg_SimilarityMap::connectSignalsToSlots()
{
	connect(horizontalSlider_WindowLower, &QSlider::valueChanged, this, &dlg_SimilarityMap::windowingChanged);
	connect(horizontalSlider_WindowUpper, &QSlider::valueChanged, this, &dlg_SimilarityMap::windowingChanged);
	connect(pushButton_LoadMap, &QPushButton::clicked, this, &dlg_SimilarityMap::loadMap);
	connect(cbShowMarkersInSpectrum, &QCheckBox::toggled, this, &dlg_SimilarityMap::showMarkers);
}

void dlg_SimilarityMap::windowingChanged( int /*val*/ )
{
	double lowerRange = ( (double)horizontalSlider_WindowLower->value() ) / horizontalSlider_WindowLower->maximum();
	double upperRange = ( (double)horizontalSlider_WindowUpper->value() ) / horizontalSlider_WindowUpper->maximum();
	m_similarityMapWidget->setWindowing( lowerRange, upperRange );
}

void dlg_SimilarityMap::connectToXRF( dlg_InSpectr* dlgXRF )
{
	m_dlgXRF = dlgXRF;
	connect( m_similarityMapWidget.data(), &iASimilarityMapWidget::energyBinsSelectedSignal, dlgXRF, &dlg_InSpectr::energyBinsSelected);
}

void dlg_SimilarityMap::loadMap()
{
	QString mapFileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open File"),
		(dynamic_cast<iAMdiChild*>(parent()))->filePath(),
		tr("MetaImages (*.mhd *.mha );;All files (*)") );
	if (mapFileName.isEmpty())
	{
		return;
	}
	m_similarityMapWidget->load(mapFileName);
}

void dlg_SimilarityMap::showMarkers(bool checked)
{
	if (checked)
	{
		m_dlgXRF->AddSimilarityMarkers();
	}
	else
	{
		m_dlgXRF->RemoveSimilarityMarkers();
	}
}
