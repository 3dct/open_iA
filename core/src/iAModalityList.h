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

#include "open_iA_Core_export.h"

#include <QSharedPointer>
#include <QVector>

class iAModality;
class iAProgress;
class iAVolumeSettings;

class vtkCamera;

typedef QVector<QSharedPointer<iAModality> > ModalityCollection;

//! Holds a list of datasets, and provides methods to save and load such lists.
class open_iA_Core_API iAModalityList : public QObject
{
	Q_OBJECT
public:
	iAModalityList();
	void store(QString const & filename, vtkCamera* cam);
	bool load(QString const & filename, iAProgress& progress);
	void applyCameraSettings(vtkCamera* cam);

	int size() const;
	QSharedPointer<iAModality> get(int idx);
	QSharedPointer<iAModality const> get(int idx) const;
	void add(QSharedPointer<iAModality> mod);
	void remove(int idx);
	QString const & fileName() const;
	static ModalityCollection load(QString const & filename, QString const & name, int channel, bool split, int renderFlags);
	bool hasUnsavedModality() const;
signals:
	void added(QSharedPointer<iAModality> mod);
private:
	bool modalityExists(QString const & filename, int channel) const;
	ModalityCollection m_modalitiesActive;
	QString m_fileName;
	bool m_camSettingsAvailable;
	double m_camPosition[3], m_camFocalPoint[3], m_camViewUp[3];
};

