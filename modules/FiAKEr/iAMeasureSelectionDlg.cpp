/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAMeasureSelectionDlg.h"

#include <QMessageBox>
#include <QStandardItemModel>

iAMeasureSelectionDlg::iAMeasureSelectionDlg(QWidget* parent) :
	QDialog(parent),
	m_model(new QStandardItemModel())
{
	this->setupUi(this);
	QStringList headers;
	headers << "Measure" << "Compute" << "Optimize" << "Use as optimization basis";
	m_model->setHorizontalHeaderLabels(headers);
	auto measureNames = getAvailableDissimilarityMeasureNames();
	for (int row = 0; row < measureNames.size(); ++row)
	{
		m_model->setItem(row, 0, new QStandardItem(measureNames[row]));
		for (int col = 1; col < 4; ++col)
		{
			auto checkItem = new QStandardItem();
			checkItem->setCheckable(true);
			checkItem->setCheckState(Qt::Unchecked);
			m_model->setItem(row, col, checkItem);
		}
	}
	lvMeasures->setModel(m_model.data());
	connect(buttonBox, &QDialogButtonBox::accepted, this, &iAMeasureSelectionDlg::okBtnClicked);
}

void iAMeasureSelectionDlg::okBtnClicked()
{
	int optimBaseCnt = 0;
	int optimCnt = 0;
	int selectedCnt = 0;
	for (int row = 0; row < m_model->rowCount(); ++row)
	{
		bool selected  = m_model->item(row, 1)->checkState() == Qt::Checked;
		bool optim     = m_model->item(row, 2)->checkState() == Qt::Checked;
		bool optimBase = m_model->item(row, 3)->checkState() == Qt::Checked;
		if (!selected && optimBase)
		{
			QMessageBox::warning(this, "FIAKER", "The measure selected as optimization base also needs to be selected for computation!");
			return;
		}
		if (optim && optimBase)
		{
			QMessageBox::warning(this, "FIAKER", "You cannot mark the same measure as requiring optimization and as optimization base!");
			return;
		}
		optimBaseCnt += (optimBase) ? 1 : 0;
		optimCnt += (optim) ? 1 : 0;
		selectedCnt += (selected) ? 1 : 0;
	}
	if (optimBaseCnt > 1)
	{
		QMessageBox::warning(this, "FIAKER", "Only one measure can be selected as optimization base!");
		return;
	}
	if (optimCnt > 0 && optimBaseCnt != 1)
	{
		QMessageBox::warning(this, "FIAKER", "If you chose to optimize a measure, you also need to select another measure as optimization base!");
		return;
	}
	if (selectedCnt == 0)
	{
		QMessageBox::warning(this, "FIAKER", "You have to choose at least one measure to compute!");
		return;
	}
	accept();
}

std::vector<std::pair<int, bool>> iAMeasureSelectionDlg::measures() const
{
	std::vector<std::pair<int, bool>> result;
	for (int row = 0; row < m_model->rowCount(); ++row)
	{
		if (m_model->item(row, 1)->checkState() == Qt::Checked)
		{
			bool optim = m_model->item(row, 2)->checkState() == Qt::Checked;
			result.push_back(std::make_pair(row, optim));
		}
	}
	return result;
}

int iAMeasureSelectionDlg::optimizeMeasureIdx() const
{
	for (int row = 0; row < m_model->rowCount(); ++row)
	{
		if (m_model->item(row, 3)->checkState() == Qt::Checked)
		{
			return row;
		}
	}
	return -1;
}