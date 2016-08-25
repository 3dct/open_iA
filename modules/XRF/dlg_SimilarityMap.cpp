/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#include "pch.h"
#include "dlg_SimilarityMap.h"

#include "iASimilarityMapWidget.h"
#include "mdichild.h"
#include "dlg_XRF.h"

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

dlg_SimilarityMap::~dlg_SimilarityMap()
{

}

void dlg_SimilarityMap::connectSignalsToSlots()
{
	connect( horizontalSlider_WindowLower, SIGNAL( valueChanged( int ) ), this, SLOT( windowingChanged( int ) ) );
	connect( horizontalSlider_WindowUpper, SIGNAL( valueChanged( int ) ), this, SLOT( windowingChanged( int ) ) );
	connect( pushButton_LoadMap, SIGNAL( clicked() ), this, SLOT( loadMap() ) );
	connect( cbShowMarkersInSpectrum, SIGNAL( toggled(bool) ), this, SLOT( showMarkers(bool) ) );
}

void dlg_SimilarityMap::windowingChanged( int val )
{
	double lowerRange = ( (double)horizontalSlider_WindowLower->value() ) / horizontalSlider_WindowLower->maximum();
	double upperRange = ( (double)horizontalSlider_WindowUpper->value() ) / horizontalSlider_WindowUpper->maximum();
	m_similarityMapWidget->setWindowing( lowerRange, upperRange );
}

void dlg_SimilarityMap::connectToXRF( dlg_XRF* dlgXRF )
{
	m_dlgXRF = dlgXRF;
	connect( m_similarityMapWidget.data(), SIGNAL( energyBinsSelectedSignal( int, int ) ), dlgXRF, SLOT( energyBinsSelected( int, int ) ) );
}

void dlg_SimilarityMap::loadMap()
{
	QString mapFileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open File"),
		(dynamic_cast<MdiChild*>(parent()))->getFilePath(),
		tr("MetaImages (*.mhd *.mha );;") );
	if(mapFileName == "")
		return;
	m_similarityMapWidget->load(mapFileName.toStdString().c_str());
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
