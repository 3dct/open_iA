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

#include <charts/iAScatterPlot.h>
#include <charts/iAQSplom.h>
#include <charts/iASPLOMData.h>
#include <iALookupTable.h>

#include <vtkDataArray.h>
#include <vtkTable.h>

#include <QAction>
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
		result->updateRanges();
		return result;
	}
}

iAFeatureScoutSPLOM::iAFeatureScoutSPLOM():
	matrix(nullptr),
	selectionEnabled(true)
{}

iAFeatureScoutSPLOM::~iAFeatureScoutSPLOM()
{
}

void iAFeatureScoutSPLOM::initScatterPlot(QDockWidget* container, vtkTable* csvTable, std::vector<char> const & columnVisibility)
{
	if (matrix)
		delete matrix;
	matrix = new iAQSplom();
	matrix->setSelectionMode(iAScatterPlot::Rectangle);
	auto spInput = createSPLOMData(csvTable);
	container->setWidget(matrix);
	matrix->showAllPlots(false);
	matrix->setData(spInput, columnVisibility);
	matrix->setSelectionColor(QColor(255, 40, 0, 1));
	matrix->enableSelection(selectionEnabled);
	matrix->showDefaultMaxizimedPlot();
	matrix->settings.enableColorSettings = true;
	connect(matrix, &iAQSplom::selectionModified, this, &iAFeatureScoutSPLOM::selectionModified);
	connect(matrix, &iAQSplom::parameterVisibilityChanged, this, &iAFeatureScoutSPLOM::parameterVisibilityChanged);

	QAction* addClass = new QAction(QObject::tr("Add class"), nullptr);
	connect(addClass, &QAction::triggered, this, &iAFeatureScoutSPLOM::addClass);
	matrix->addContextMenuAction(addClass);
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

void iAFeatureScoutSPLOM::updateColumnVisibility(std::vector<char> const & columnVisibility)
{
	if (matrix)
		matrix->setParameterVisibility(columnVisibility);
}

void iAFeatureScoutSPLOM::setParameterVisibility(size_t paramIndex, bool visible)
{
	if (matrix)
		matrix->setParameterVisibility(paramIndex, visible);
}

void iAFeatureScoutSPLOM::setFilter(int classID)
{
	if (!matrix)
		return;
	matrix->resetFilter();
	if (classID != -1)
		matrix->addFilter(matrix->data()->numParams() - 1, classID);
	matrix->update();
}

void iAFeatureScoutSPLOM::setDotColor(QColor const & color)
{
	if (!matrix)
		return;
	matrix->setPointColor(color);
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
	if (!matrix)
		return;
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
	if (!matrix)
		return;
	size_t classColumn = matrix->data()->numParams() - 1;
	matrix->data()->data()[classColumn][objID] = classID;
}

void iAFeatureScoutSPLOM::classesChanged()
{
	if (!matrix)
		return;
	size_t classColumn = matrix->data()->numParams() - 1;
	matrix->data()->updateRange(classColumn);
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
