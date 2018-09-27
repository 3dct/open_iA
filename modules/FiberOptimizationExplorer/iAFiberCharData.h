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

#include "iA3DCylinderObjectVis.h"

#include "iAProgress.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QThread>

#include <vector>

class iASPLOMData;

class iACsvConfig;

class vtkTable;

class QCheckBox;

//! fibervalues layout unless otherwise specified : (start[x, y, z], end[x, y, z], center[x, y, z], phi, theta, length, diameter)

//! Class for holding data about the distances from one result to another fiber in a different dataset
class iAFiberDistance
{
public:
	size_t index;
	double distance;
	friend bool operator<(iAFiberDistance const & a, iAFiberDistance const & b);
};

//! Comparison data to reference for a single timestep, fiber and result
class iARefDiffFiberTimeData
{
public:
	//! diff of fibervalues (+distances)
	std::vector<double> diff;
};

//! Comparison data to reference for a single fiber in a result
class iARefDiffFiberData
{
public:
	//! differences to reference fiber, stored per timestep
	std::vector<iARefDiffFiberTimeData> timeStep;
	//! dist to ref fibers in order of ascending difference
	std::vector<std::vector<iAFiberDistance> > dist;
};

//! Data for a single fiber characterization result
class iAFiberCharData
{
public:
	static const int FiberValueCount = 13;
	//! the fiber data as vtkTable, mainly for the 3d visualization:
	vtkSmartPointer<vtkTable> table;
	//! mapping of the columns in m_resultTable
	QSharedPointer<QMap<uint, uint> > mapping;
	//! name of the csv file this result was loaded from
	QString fileName;
	//! values for all timesteps, stored as: timestep, fiber, fibervalues
	std::vector<std::vector<std::vector<double> > > timeValues;
	//! projection error stored as fiber, timestep, global projection error
	std::vector<std::vector<double > > projectionError;
	//! comparison data to reference for each fiber
	std::vector<iARefDiffFiberData> refDiffFiber;
	//! number of fibers in the dataset:
	size_t fiberCount;
};

class iAFiberResultsCollection
{
public:
	static const QString LegacyFormat;
	static const QString SimpleFormat;
	iAFiberResultsCollection();
	std::vector<iAFiberCharData> results;
	QSharedPointer<iASPLOMData> splomData;
	size_t minFiberNumber;
	int timeStepMax;
	bool loadData(QString const & path, QString const & configName, iAProgress * progress);
};

class iAFiberResultsLoader: public QThread
{
	Q_OBJECT
public:
	iAFiberResultsLoader(QSharedPointer<iAFiberResultsCollection> results, QString const & path, QString const & configName);
	void run() override;
	iAProgress* progress();
signals:
	void failed(QString const & path);
private:
	iAProgress m_progress;
	QSharedPointer<iAFiberResultsCollection> m_results;
	QString m_path;
	QString m_configName;
};

// helper functions:
void addColumn(vtkSmartPointer<vtkTable> table, float value, char const * columnName, size_t numRows);
iACsvConfig getCsvConfig(QString const & csvFile, QString const & formatName);
