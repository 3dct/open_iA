/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

// charts
#include <iAScatterPlot.h>
#include <iAQSplom.h>
#include <iASPLOMData.h>

// base
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
		{
			paramNames.push_back(table->GetColumnName(col));
		}
		result->setParameterNames(paramNames);
		for (vtkIdType row = 0; row < table->GetNumberOfRows(); ++row)
		{
			for (vtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
			{
				double value = table->GetValue(row, col).ToDouble();
				result->data()[col].push_back(value);
			}
		}
		result->updateRanges();
		return result;
	}
}

iAFeatureScoutSPLOM::iAFeatureScoutSPLOM():
	matrix(nullptr),
	selectionEnabled(true)
{}

void iAFeatureScoutSPLOM::initScatterPlot(vtkTable* csvTable, std::vector<char> const & columnVisibility)
{
	assert(!matrix);
	matrix = new iAQSplom();
	matrix->setSelectionMode(iAScatterPlot::Rectangle);
	auto spInput = createSPLOMData(csvTable);
	matrix->showAllPlots(false);
	matrix->setData(spInput, columnVisibility);
	matrix->setSelectionColor(QColor(255, 40, 0, 255));
	matrix->enableSelection(selectionEnabled);
	matrix->showDefaultMaxizimedPlot();
	matrix->settings.enableColorSettings = true;
	connect(matrix, &iAQSplom::selectionModified, this, &iAFeatureScoutSPLOM::selectionModified);
	connect(matrix, &iAQSplom::parameterVisibilityChanged, this, &iAFeatureScoutSPLOM::parameterVisibilityChanged);
	connect(matrix, &iAQSplom::lookupTableChanged, this, &iAFeatureScoutSPLOM::lookupTableChanged);

	QAction* addClass = new QAction(tr("Add class"), nullptr);
	connect(addClass, &QAction::triggered, this, &iAFeatureScoutSPLOM::addClass);
	matrix->addContextMenuAction(addClass);
}

void iAFeatureScoutSPLOM::multiClassRendering(QList<QColor> const & colors)
{
	if (!matrix)
	{
		return;
	}
	matrix->viewData()->clearFilters();
	iALookupTable lookupTable;
	lookupTable.allocate(colors.size());
	lookupTable.setRange(0, colors.size() - 1);
	for (int c = 0; c < colors.size(); ++c)
	{
		lookupTable.setColor(static_cast<size_t>(c), colors.at(c));
	}
	matrix->setLookupTable(lookupTable, matrix->data()->numParams()-1 );
}

void iAFeatureScoutSPLOM::updateColumnVisibility(std::vector<char> const & columnVisibility)
{
	if (matrix)
	{
		matrix->setParameterVisibility(columnVisibility);
	}
}

void iAFeatureScoutSPLOM::setParameterVisibility(size_t paramIndex, bool visible)
{
	if (matrix)
	{
		matrix->setParameterVisibility(paramIndex, visible);
	}
}

void iAFeatureScoutSPLOM::lookupTableChanged()
{
	emit renderLUTChanges(matrix->lookupTable(), matrix->colorLookupParam());
}

void iAFeatureScoutSPLOM::setFilter(int classID)
{
	if (!matrix)
	{
		return;
	}
	matrix->viewData()->clearFilters();
	if (classID != -1)
	{
		matrix->viewData()->addFilter(matrix->data()->numParams() - 1, classID);
	}
	matrix->update();
}

void iAFeatureScoutSPLOM::setDotColor(QColor const & color)
{
	if (!matrix)
	{
		return;
	}
	matrix->setPointColor(color);
}

void iAFeatureScoutSPLOM::setFilteredSelection(std::vector<size_t> const & selection)
{
	if (matrix && selectionEnabled)
	{
		matrix->viewData()->setFilteredSelection(selection, matrix->data());
	}
}

void iAFeatureScoutSPLOM::classAdded(int classID)
{
	if (!matrix)
	{
		return;
	}
	size_t classColumn = matrix->data()->numParams() - 1;
	auto sel = matrix->viewData()->selection();
	for (size_t objID : sel)
	{
		matrix->data()->data()[classColumn][objID] = classID;
	}
	classesChanged();
	setFilter(classID);
	matrix->viewData()->clearSelection();
	matrix->update();
}

void iAFeatureScoutSPLOM::classDeleted(int deleteClassID)
{
	if (!matrix)
	{
		return;
	}
	matrix->viewData()->clearSelection();
	size_t classColumn = matrix->data()->numParams() - 1;
	for (size_t objID = 0; objID < matrix->data()->numPoints(); ++objID)
	{
		int curClassID = matrix->data()->paramData(classColumn)[objID];
		if (curClassID == deleteClassID)
		{
			matrix->data()->data()[classColumn][objID] = 0;
		}
		else if (curClassID > deleteClassID)
		{
			matrix->data()->data()[classColumn][objID] = curClassID - 1;
		}
	}
	classesChanged();
	setFilter(0); // set filter to unclassified class
	matrix->update();
}

void iAFeatureScoutSPLOM::changeClass(size_t objID, int classID)
{
	if (!matrix)
	{
		return;
	}
	size_t classColumn = matrix->data()->numParams() - 1;
	matrix->data()->data()[classColumn][objID] = classID;
}

void iAFeatureScoutSPLOM::classesChanged()
{
	if (!matrix)
	{
		return;
	}
	size_t classColumn = matrix->data()->numParams() - 1;
	matrix->data()->updateRange(classColumn);
}

std::vector<size_t> iAFeatureScoutSPLOM::getFilteredSelection() const
{
	return matrix->viewData()->filteredSelection(matrix->data());
}

bool iAFeatureScoutSPLOM::isShown() const
{
	return matrix;
}

void iAFeatureScoutSPLOM::clearSelection()
{
	if (matrix)
	{
		matrix->viewData()->clearSelection();
	}
}

void iAFeatureScoutSPLOM::enableSelection(bool enable)
{
	selectionEnabled = enable;
	if (!matrix)
	{
		return;
	}
	matrix->viewData()->clearSelection();
	matrix->enableSelection(enable);
}

QWidget* iAFeatureScoutSPLOM::matrixWidget()
{
	return matrix;
}
