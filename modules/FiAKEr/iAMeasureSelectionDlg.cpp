// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMeasureSelectionDlg.h"

#include "iAFiberData.h"

#include "iALog.h"

#include <QMessageBox>

iAMeasureSelectionDlg::iAMeasureSelectionDlg(QWidget* parent) :
	QDialog(parent),
	m_model(new QStandardItemModel())
{
	this->setupUi(this);
	QStringList headers;
	headers << "Measure" << "Compute" << "Optimize" << "Optim. base" << "Best Measure";
	m_model->setHorizontalHeaderLabels(headers);
	auto measureNames = getAvailableDissimilarityMeasureNames();
	for (int row = 0; row < measureNames.size(); ++row)
	{
		m_model->setItem(row, 0, new QStandardItem(measureNames[row]));
		for (int col = 1; col < 5; ++col)
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
	int bestMeasCnt = 0;
	for (int row = 0; row < m_model->rowCount(); ++row)
	{
		bool selected  = m_model->item(row, 1)->checkState() == Qt::Checked;
		bool optim     = m_model->item(row, 2)->checkState() == Qt::Checked;
		bool optimBase = m_model->item(row, 3)->checkState() == Qt::Checked;
		bool bestMeas  = m_model->item(row, 4)->checkState() == Qt::Checked;
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
		if (!selected & bestMeas)
		{
			QMessageBox::warning(this, "FIAKER", "The measure selected as best also needs to be selected for computation!");
			return;
		}
		optimBaseCnt += (optimBase) ? 1 : 0;
		optimCnt += (optim) ? 1 : 0;
		selectedCnt += (selected) ? 1 : 0;
		bestMeasCnt += (bestMeas) ? 1 : 0;
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
	if (bestMeasCnt != 1)
	{
		QMessageBox::warning(this, "FIAKER", "You have to choose exactly one 'best measure'!");
		return;
	}
	accept();
}

iAMeasureSelectionDlg::TMeasureSelection iAMeasureSelectionDlg::measures() const
{
	TMeasureSelection result;
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
			auto m = measures();
			for (size_t mIdx = 0; mIdx < m.size(); ++mIdx)
			{
				if (m[mIdx].first == row)
				{	// assuming that there won't be more than 2billion measures is probably safe
					return static_cast<int>(mIdx);
				}
			}
			LOG(lvlWarn, "Optimization measure requested, but not selected to be computed!");
			break;
		}
	}
	return -1;
}

int iAMeasureSelectionDlg::bestMeasureIdx() const
{
	for (int row = 0; row < m_model->rowCount(); ++row)
	{
		if (m_model->item(row, 4)->checkState() == Qt::Checked)
		{
			return row;
		}
	}
	return -1;
}
