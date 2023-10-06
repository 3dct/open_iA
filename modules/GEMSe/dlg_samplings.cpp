// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_samplings.h"

#include "iASamplingResults.h"

#include <QStandardItemModel>

dlg_samplings::dlg_samplings():
	m_itemModel(new QStandardItemModel()),
	m_samplings(new QVector<SamplingResultPtr>())
{
	connect(pbAdd, &QPushButton::clicked, this, &dlg_samplings::AddSampling);
	connect(pbRemove, &QPushButton::clicked, this, &dlg_samplings::Remove);
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Samplings"));
	lvSamplings->setModel(m_itemModel);
}

std::shared_ptr<iASamplingResults> dlg_samplings::GetSampling(int idx)
{
	return m_samplings->at(idx);
}

void dlg_samplings::Add(std::shared_ptr<iASamplingResults> samplingResults)
{
	QStandardItem* newItem = new QStandardItem(samplingResults->fileName());
	m_samplings->push_back(samplingResults);
	m_itemModel->appendRow(newItem);
}

void dlg_samplings::Remove()
{
	QModelIndexList indices = lvSamplings->selectionModel()->selectedIndexes();
	if (indices.size() == 0)
	{
		return;
	}
	QStandardItem* item = m_itemModel->itemFromIndex(indices[0]);
	int curRow = item->row();
	if (curRow == -1)
	{
		return;
	}
	m_itemModel->removeRow(curRow);
	m_samplings->erase(m_samplings->begin() + curRow);
}

int dlg_samplings::SamplingCount() const
{
	return m_samplings->size();
}

std::shared_ptr<QVector<dlg_samplings::SamplingResultPtr>> dlg_samplings::GetSamplings()
{
	return m_samplings;
}
