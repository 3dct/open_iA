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
#include "iAFeatureScoutSPLOM.h"

#include "charts/iAScatterPlot.h"
#include "charts/iAQSplom.h"
#include "charts/iASPLOMData.h"
#include "iALookupTable.h"

#include <vtkDataArray.h>
#include <vtkTable.h>

#include <QDockWidget>

namespace
{
	QSharedPointer<iASPLOMData> createSPLOMData(vtkTable* table)
	{
		QSharedPointer<iASPLOMData> result(new iASPLOMData());
		std::vector<QString> paramNames;
		for (vtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
			paramNames.push_back(table->GetColumnName(col));
		result->setParameterNames(paramNames);
		for (vtkIdType row = 0; row < table->GetNumberOfRows(); ++row)
			for (vtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
			{
				double value = table->GetValue(row, col).ToDouble();
				result->data()[col].push_back(value);
			}
		return result;
	}
}

iAFeatureScoutSPLOM::iAFeatureScoutSPLOM():
	matrix(nullptr),
	selectionEnabled(true)
{}

iAFeatureScoutSPLOM::~iAFeatureScoutSPLOM()
{
	delete matrix;
}

void iAFeatureScoutSPLOM::initScatterPlot(QDockWidget* container, vtkTable* csvTable)
{
	if (matrix)
		delete matrix;
	matrix = new iAQSplom();
	matrix->setSelectionMode(iAScatterPlot::Rectangle);
	auto spInput = createSPLOMData(csvTable);
	container->setWidget(matrix);
	matrix->setData(spInput);
	matrix->setSelectionColor(QColor(255, 40, 0, 1));
	matrix->enableSelection(selectionEnabled);
	matrix->showDefaultMaxizimedPlot();
	connect(matrix, &iAQSplom::selectionModified, this, &iAFeatureScoutSPLOM::selectionModified);
}

void iAFeatureScoutSPLOM::multiClassRendering(QList<QColor> const & colors)
{
	if (!matrix)
		return;
	matrix->resetFilter();
	iALookupTable lookupTable;
	lookupTable.allocate(colors.size());
	lookupTable.setRange(0, colors.size() - 1);
	for (size_t c = 0; c < colors.size(); ++c)
		lookupTable.setColor(c, colors.at(c));
	matrix->setLookupTable(lookupTable, matrix->data()->numParams()-1 );
}

void iAFeatureScoutSPLOM::updateColumnVisibility(std::vector<bool> const & columnVisibility)
{
	if (matrix)
		matrix->setParameterVisibility(columnVisibility);
}

void iAFeatureScoutSPLOM::setFilter(int classID)
{
	if (!matrix)
		return;
	if (classID == -1)
		matrix->resetFilter();
	else
	{
		matrix->setFilter(matrix->data()->numParams() - 1, classID);
	}
	matrix->update();
}

void iAFeatureScoutSPLOM::setDotColor(QColor const & color, double const range[2])
{
	iALookupTable lut;
	lut.setRange(range);
	lut.allocate(2);
	for (size_t i = 0; i < 2; i++)
		lut.setColor(i, color);
	matrix->setLookupTable(lut, 0);
}

void iAFeatureScoutSPLOM::setFilteredSelection(std::vector<size_t> const & selection)
{
	if (matrix && selectionEnabled)
		matrix->setFilteredSelection(selection);
}

void iAFeatureScoutSPLOM::classAdded(int classID)
{
	if (!matrix)
		return;
	size_t classColumn = matrix->data()->numParams() - 1;
	auto sel = matrix->getSelection();
	for (size_t objID : sel)
		matrix->data()->data()[ classColumn ][ objID ] = classID;
	classesChanged();
	setFilter(classID);
	matrix->clearSelection();
	matrix->update();
}

void iAFeatureScoutSPLOM::classDeleted(int deleteClassID)
{
	matrix->clearSelection();
	size_t classColumn = matrix->data()->numParams() - 1;
	for (size_t objID = 0; objID < matrix->data()->numPoints(); ++objID)
	{
		int curClassID = matrix->data()->paramData(classColumn)[objID];
		if (curClassID == deleteClassID)
			matrix->data()->data()[classColumn][objID] = 0;
		else if (curClassID > deleteClassID)
			matrix->data()->data()[classColumn][objID] = curClassID-1;
	}
	classesChanged();
	setFilter(0); // set filter to unclassified class
	matrix->update();
}

void iAFeatureScoutSPLOM::changeClass(size_t objID, int classID)
{
	size_t classColumn = matrix->data()->numParams() - 1;
	matrix->data()->data()[classColumn][objID] = classID;
}

void iAFeatureScoutSPLOM::classesChanged()
{
	size_t classColumn = matrix->data()->numParams() - 1;
	matrix->paramChanged(classColumn);
}

std::vector<size_t> iAFeatureScoutSPLOM::getFilteredSelection() const
{
	return matrix->getFilteredSelection();
}

bool iAFeatureScoutSPLOM::isShown() const
{
	return matrix;
}

void iAFeatureScoutSPLOM::clearSelection()
{
	if (matrix)
		matrix->clearSelection();
}

void iAFeatureScoutSPLOM::enableSelection(bool enable)
{
	selectionEnabled = enable;
	if (!matrix)
		return;
	matrix->clearSelection();
	matrix->enableSelection(enable);
}
