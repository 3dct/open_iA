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
#include "qcustomplot.h"

#include <QDockWidget>
#include <QDir>

template <typename ArgType, typename ValType>
class iAFunctionalBoxplot;
typedef iAFunctionalBoxplot< unsigned int, double> FunctionalBoxPlot;

struct icData
{
	double intensity;
	unsigned int x;
	unsigned int y;
	unsigned int z;
};

enum PathID
{
	P_HILBERT,
	P_SCAN_LINE
};

const QStringList pathNames = QStringList()\
<< "Hilbert"\
<< "Scan Line";

typedef QMap<QString, PathID> MapPathNames2PathID;
static MapPathNames2PathID fill_PathNameToId()
{
	MapPathNames2PathID m;
	m[pathNames.at(0)] = P_HILBERT;
	m[pathNames.at(1)] = P_SCAN_LINE;

	return m;
}
const MapPathNames2PathID PathNameToId = fill_PathNameToId();

typedef iAQTtoUIConnector<QDockWidget, Ui_dlg_DatasetComparator>  DatasetComparatorConnector;

class dlg_DatasetComparator : public DatasetComparatorConnector
{
	Q_OBJECT

public:
	dlg_DatasetComparator(QWidget * parent, QDir datasetsDir, Qt::WindowFlags f = 0);
	~dlg_DatasetComparator();

	QDir m_datasetsDir;
	QList<QPair <QString, QList<icData> >> m_DatasetIntensityMap;

public slots:
	void mousePress(QMouseEvent*);
	void mouseMove(QMouseEvent*);
	void selectionChanged(const QCPDataSelection & selection);
	void setFbpTransparency(int);
	void showFBPGraphs();
	void showLinePlots();
	void updateDatasetComparator();
	void updateFBPView();
	void visualizePath();

private:
	MdiChild * m_mdiChild;
	QCustomPlot * m_customPlot;
	QCPItemText * m_dataPointInfo;

	void mapIntensities();
	void createFBPGraphs(iAFunctionalBoxplot<unsigned int, double>* fbpData);
};
