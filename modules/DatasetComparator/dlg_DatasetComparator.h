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

#include "Ui_dlg_DatasetComparator.h"
#include "iAQTtoUIConnector.h"
#include "mdichild.h"

#include <itkHilbertPath.h>

#include <QDockWidget>
#include <QDir>

typedef iAQTtoUIConnector<QDockWidget, Ui_dlg_DatasetComparator>  DatasetComparatorConnector;
typedef itk::HilbertPath<unsigned int, 3> PathType;

class dlg_DatasetComparator : public DatasetComparatorConnector
{
	Q_OBJECT

public:
	dlg_DatasetComparator(QWidget * parent, QDir datasetsDir, Qt::WindowFlags f = 0);
	~dlg_DatasetComparator();

	PathType::Pointer m_HPath;
	QDir m_datasetsDir;
	QMap<QString, QList<int> > m_DatasetIntensityMap;

public slots:
	void showHilbertLinePlots();
	void visualizeHilbertPath();

private:
	MdiChild * m_mdiChild;
};
