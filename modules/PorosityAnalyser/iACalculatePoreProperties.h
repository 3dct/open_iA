/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include "ui_CalculatePoreProperties.h"

#include <QThread>
#include <QTableWidget>

const int numThreads = 4;

class iACalculatePorePropertiesThread : public QThread
{
	Q_OBJECT
public:
	iACalculatePorePropertiesThread( QTableWidget * masks, QObject * parent = 0 ) : QThread( parent ), m_masks( masks ){};
protected:
	virtual void run();
signals:
	void totalProgress( int progress );
protected:
	QTableWidget * m_masks;
};

#include "iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QWidget, Ui_calculatePoreProperties> PorePropertiesConnector;

class iACalculatePoreProperties : public PorePropertiesConnector
{
	Q_OBJECT

public:
	iACalculatePoreProperties( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	void SetMasksCSVPath( QString masksCSVPath );
	void CalculatePoreProperties();
	virtual ~iACalculatePoreProperties();

protected slots:
	void totalProgressSlot( int progress );
	void browseCSV();
	
protected:
	void LoadSettings();
	void SaveSettings();

	iACalculatePorePropertiesThread * m_calcThread[numThreads];
	QTableWidget m_splitMasks[numThreads];
	QTableWidget m_masks;
	QString m_masksCSVPath;
};
