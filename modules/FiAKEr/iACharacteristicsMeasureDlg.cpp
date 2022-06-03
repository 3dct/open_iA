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
#include "iACharacteristicsMeasureDlg.h"

#include "iAFiberData.h"
#include "iAFiberResult.h"

#include "ui_CharacteristicsMeasureDialog.h"

#include <iASPLOMData.h>

#include <QStandardItemModel>


QStringList const& DistributionDifferenceMeasureNames()
{
	static QStringList Names = QStringList() << "L2 Difference" << "Jensen-Shannon divergence";
	return Names;
}

namespace
{
	void addCheckItem(QStandardItemModel* model, int i, QString const& title, bool checkEnabled)
	{
		model->setItem(i, 0, new QStandardItem(title));
		auto checkItem = new QStandardItem();
		checkItem->setCheckable(true);
		checkItem->setEnabled(checkEnabled);
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

iACharacteristicsMeasureDlg::iACharacteristicsMeasureDlg(
	QSharedPointer<iAFiberResultsCollection> data) :
	m_characteristicsModel(new QStandardItemModel()),
	m_diffMeasuresModel(new QStandardItemModel()),
	m_ui(new Ui_CharacteristicsMeasureDialog())
{
	m_ui->setupUi(this);
	m_characteristicsModel->setHorizontalHeaderLabels(QStringList() << "Characteristic" << "Select" << "Min" << "Max");
	for (int i = 0; i < static_cast<int>(data->m_resultIDColumn); ++i)
	{
		addCheckItem(m_characteristicsModel, i, data->spmData->parameterName(i),
			data->spmData->paramRange(i)[0] != data->spmData->paramRange(i)[1]);
		m_characteristicsModel->setItem(i, 2, new QStandardItem(QString::number(data->spmData->paramRange(i)[0])));
		m_characteristicsModel->setItem(i, 3, new QStandardItem(QString::number(data->spmData->paramRange(i)[1])));
	}
	m_ui->tvCharacteristic->setModel(m_characteristicsModel);
	m_ui->tvCharacteristic->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->tvCharacteristic->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_ui->tvCharacteristic->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	m_ui->tvCharacteristic->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

	m_diffMeasuresModel->setHorizontalHeaderLabels(QStringList() << "Difference" << "Select");
	// Difference Between Mean, Min, Max ? other single measures?
	for (int i = 0; i < DistributionDifferenceMeasureNames().size(); ++i)
	{
		addCheckItem(m_diffMeasuresModel, i, DistributionDifferenceMeasureNames()[i], true);
	}
	//addCheckItem(diffMeasuresModel, 2, "Mutual information");
	// ... some other measures from iAVectorDistance...?
	m_ui->tvDiffMeasures->setModel(m_diffMeasuresModel);
	m_ui->tvDiffMeasures->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->tvDiffMeasures->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

QVector<int> iACharacteristicsMeasureDlg::selectedCharacteristics() const
{
	return selectedIndices(m_characteristicsModel);
}

QVector<int> iACharacteristicsMeasureDlg::selectedDiffMeasures() const
{
	return selectedIndices(m_diffMeasuresModel);
}
