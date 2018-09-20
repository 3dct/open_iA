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

#include <QMap>
#include <QSharedPointer>

#include <vtkSmartPointer.h>

#include <vector>

class vtkTable;

class QCheckBox;

class iAFiberDistance
{
public:
	size_t index;
	double distance;
	friend bool operator<(iAFiberDistance const & a, iAFiberDistance const & b);
};

class iAFiberCharData
{
public:
	// data:
	vtkSmartPointer<vtkTable> m_resultTable;
	QSharedPointer<QMap<uint, uint> > m_outputMapping;
	QString m_fileName;
	// timestep, fiber, 6 values (center, phi, theta, length)
	std::vector<std::vector<std::vector<double> > > m_timeValues;
	// fiber, timestep, global projection error
	std::vector<QSharedPointer<std::vector<double> > > m_projectionError;
	size_t m_startPlotIdx;
	// fiber, errors (shiftx, shifty, shiftz, phi, theta, length, diameter)
	// TODO: compute + visualize per timestep!
	std::vector<std::vector<double> > m_referenceDiff;
	// fiber, distance measure, match in reference
	std::vector<std::vector<std::vector<iAFiberDistance> > > m_referenceDist;
	size_t m_fiberCount;

	// UI elements:
	iAVtkWidgetClass* m_vtkWidget;
	QSharedPointer<iA3DCylinderObjectVis> m_mini3DVis;
	QSharedPointer<iA3DCylinderObjectVis> m_main3DVis;
	QCheckBox* m_boundingBox;
};
