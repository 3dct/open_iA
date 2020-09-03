/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "dlg_SimilarityMap.h"

#include "dlg_InSpectr.h"

#include <mdichild.h>

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
		(dynamic_cast<MdiChild*>(parent()))->filePath(),
		tr("MetaImages (*.mhd *.mha );;") );
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
