/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "pch.h"
#include "dlg_samplings.h"

#include "iASamplingResults.h"

#include <QStandardItemModel>

dlg_samplings::dlg_samplings():
	m_itemModel(new QStandardItemModel())
{
	connect(pbRemove, SIGNAL(clicked()), this, SLOT(Remove()));
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Samplings"));
	lvSamplings->setModel(m_itemModel);
}

QSharedPointer<iASamplingResults> dlg_samplings::GetSampling(int idx)
{
	return m_samplings[idx];
}

void dlg_samplings::Add(QSharedPointer<iASamplingResults> samplingResults)
{
	QStandardItem* newItem = new QStandardItem(samplingResults->GetFileName());
	m_samplings.push_back(samplingResults);
	m_itemModel->appendRow(newItem);
}

void dlg_samplings::Remove()
{
	QModelIndexList indices = lvSamplings->selectionModel()->selectedIndexes();
	if (indices.size() < 0)
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
	m_samplings.erase(m_samplings.begin() + curRow);
}

int dlg_samplings::SamplingCount() const
{
	return m_itemModel->rowCount();
}

QVector<dlg_samplings::SamplingResultPointer> const & dlg_samplings::GetSamplings()
{
	return m_samplings;
}
