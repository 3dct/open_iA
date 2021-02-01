/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASensitivityDialog.h"

#include "iAFiberData.h"
#include "iAFiberCharData.h"

#include <iASPLOMData.h>

#include <QStandardItemModel>

QStringList const& DistributionDifferenceMeasureNames()
{
	static QStringList Names = QStringList() << "L2 Difference" << "Jensen-Shannon divergence";
	return Names;
}

namespace
{
	void addCheckItem(QStandardItemModel* model, int i, QString const& title)
	{
		model->setItem(i, 0, new QStandardItem(title));
		auto checkItem = new QStandardItem();
		checkItem->setCheckable(true);
		checkItem->setCheckState(Qt::Unchecked);
		model->setItem(i, 1, checkItem);
	}

	QVector<int> selectedIndices(QStandardItemModel const* model)
	{
		QVector<int> result;
		for (int row = 0; row < model->rowCount(); ++row)
		{
			if (model->item(row, 1)->checkState() == Qt::Checked)
			{
				result.push_back(row);
			}
		}
		return result;
	}
}

iASensitivityDialog::iASensitivityDialog(QSharedPointer<iAFiberResultsCollection> data) :
	m_characteristicsModel(new QStandardItemModel()),
	m_diffMeasuresModel(new QStandardItemModel()),
	m_measuresModel(new QStandardItemModel())
{
	m_characteristicsModel->setHorizontalHeaderLabels(QStringList() << "Characteristic" << "Select");
	for (int i = 0; i < static_cast<int>(data->m_resultIDColumn); ++i)
	{
		addCheckItem(m_characteristicsModel, i, data->spmData->parameterName(i));
	}
	tvCharacteristic->setModel(m_characteristicsModel);
	tvCharacteristic->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	tvCharacteristic->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	m_diffMeasuresModel->setHorizontalHeaderLabels(QStringList() << "Difference" << "Select");
	// Difference Between Mean, Min, Max ? other single measures?
	for (int i = 0; i < DistributionDifferenceMeasureNames().size(); ++i)
	{
		addCheckItem(m_diffMeasuresModel, i, DistributionDifferenceMeasureNames()[i]);
	}
	//addCheckItem(diffMeasuresModel, 2, "Mutual information");
	// ... some other measures from iAVectorDistance...?
	tvDiffMeasures->setModel(m_diffMeasuresModel);
	tvDiffMeasures->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	tvDiffMeasures->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	m_measuresModel->setHorizontalHeaderLabels(QStringList() << "Measure" << "Select");
	auto measureNames = getAvailableDissimilarityMeasureNames();
	for (int i = 0; i < measureNames.size(); ++i)
	{
		addCheckItem(m_measuresModel, i, measureNames[i]);
	}
	tvMeasures->setModel(m_measuresModel);
	tvMeasures->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	tvMeasures->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

QVector<int> iASensitivityDialog::selectedCharacteristics() const
{
	return selectedIndices(m_characteristicsModel);
}

QVector<int> iASensitivityDialog::selectedMeasures() const
{
	return selectedIndices(m_measuresModel);
}

QVector<int> iASensitivityDialog::selectedDiffMeasures() const
{
	return selectedIndices(m_diffMeasuresModel);
}
