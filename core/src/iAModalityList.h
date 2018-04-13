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
#pragma once

#include "open_iA_Core_export.h"

#include <QSharedPointer>
#include <QVector>

class iAModality;
class iAVolumeSettings; 

class vtkCamera;

typedef QVector<QSharedPointer<iAModality> > ModalityCollection;


class open_iA_Core_API iAModalityList : public QObject
{
	Q_OBJECT
public:
	iAModalityList();
	void Store(QString const & filename, vtkCamera* cam);
	bool Load(QString const & filename);
	void ApplyCameraSettings(vtkCamera* cam);

	int size() const;
	QSharedPointer<iAModality> Get(int idx);
	QSharedPointer<iAModality const> Get(int idx) const;
	void Add(QSharedPointer<iAModality> mod);
	void Remove(int idx);
	QString const & GetFileName() const;
	static ModalityCollection Load(QString const & filename, QString const & name, int channel, bool split, int renderFlags);
	bool HasUnsavedModality() const;
signals:
	void Added(QSharedPointer<iAModality> mod);
private:
	bool ModalityExists(QString const & filename, int channel) const;


	void checkandSetVolumeSettings(iAVolumeSettings &volSettings, const QString &Shading, const QString &LinearInterpolation,
		const QString &SampleDistance, const QString AmbientLighting, const QString& DiffuseLighting, const QString &SpecularLighting,
		const QString &SpecularPower); 

	ModalityCollection m_modalities;
	QString m_fileName;
	bool m_camSettingsAvailable;
	double camPosition[3], camFocalPoint[3], camViewUp[3];
};

