/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "ui_SimilarityMap.h"

#include "iASimilarityMapWidget.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QScopedPointer>

class dlg_XRF;

typedef iAQTtoUIConnector<QDockWidget, Ui_SimilarityMap>   dlg_SimilarityMapContainer;

class dlg_SimilarityMap : public dlg_SimilarityMapContainer
{
	Q_OBJECT
public:
	dlg_SimilarityMap( QWidget *parentWidget = 0 );
	void connectToXRF( dlg_XRF* dlgXRF );
protected:
	void connectSignalsToSlots();
protected slots:
	void windowingChanged( int val = 0);
	void loadMap();
	void showMarkers(bool checked);
protected:
	dlg_XRF* m_dlgXRF;
	QScopedPointer<iASimilarityMapWidget> m_similarityMapWidget;
	QGridLayout * m_similarityWidgetGridLayout;
};
