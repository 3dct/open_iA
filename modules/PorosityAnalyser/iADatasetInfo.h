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
#pragma once

#include "PorosityAnalyserHelpers.h"    // for ImagePointer

#include <QThread>

class iAPorosityAnalyserModuleInterface;

class iADatasetInfo : public QThread
{
	Q_OBJECT

public:
	iADatasetInfo( iAPorosityAnalyserModuleInterface * pmi, QObject * parent = 0 ) : m_pmi( pmi ), QThread( parent ) {};
	QStringList getNewGeneratedInfoFiles();

protected:
	virtual void run();
	void calculateInfo();

	iAPorosityAnalyserModuleInterface * m_pmi;

signals:
	void progress( int );

private:
	template<class T> void generateInfo( QString datasetPath, QString datasetName,
										ImagePointer & image, iAPorosityAnalyserModuleInterface * pmi,
										int filesInfoNbToCreate, int currentFInfoNb );

	QStringList m_newGeneratedInfoFilesList;
};
