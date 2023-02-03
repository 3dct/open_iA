// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_SimilarityMap.h"

#include "iASimilarityMapWidget.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QScopedPointer>

class dlg_InSpectr;

typedef iAQTtoUIConnector<QDockWidget, Ui_SimilarityMap>   dlg_SimilarityMapContainer;

class dlg_SimilarityMap : public dlg_SimilarityMapContainer
{
	Q_OBJECT
public:
	dlg_SimilarityMap( QWidget *parentWidget = 0 );
	void connectToXRF( dlg_InSpectr* dlgXRF );
protected:
	void connectSignalsToSlots();
protected slots:
	void windowingChanged( int val = 0);
	void loadMap();
	void showMarkers(bool checked);
protected:
	dlg_InSpectr* m_dlgXRF;
	QScopedPointer<iASimilarityMapWidget> m_similarityMapWidget;
	QGridLayout * m_similarityWidgetGridLayout;
};
