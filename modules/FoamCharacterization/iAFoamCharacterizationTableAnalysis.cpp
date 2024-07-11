// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationTableAnalysis.h"

#include "iAStringHelper.h"

#include <QHeaderView>
#include <QStandardItemModel>

iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::CTableAnalysisRow()
{
}

void iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::set(const long& lLabel
	, const double& dCenterX, const double& dCenterY, const double& dCenterZ
	, const double& dVolume, const double& dDiameter)
{
	m_lLabel = lLabel;

	m_dCenterX = dCenterX;
	m_dCenterY = dCenterY;
	m_dCenterZ = dCenterZ;

	m_dVolume = dVolume;
	m_dDiameter = dDiameter;
}

long iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::label() const
{
	return m_lLabel;
}

double iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::centerX() const
{
	return m_dCenterX;
}

double iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::centerY() const
{
	return m_dCenterY;
}

double iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::centerZ() const
{
	return m_dCenterZ;
}

double iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::volume() const
{
	return m_dVolume;
}

double iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::diameter() const
{
	return m_dDiameter;
}


iAFoamCharacterizationTableAnalysis::iAFoamCharacterizationTableAnalysis(QWidget* _pParent) : QTableView (_pParent)
{
	setCursor(Qt::PointingHandCursor);

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setFocusPolicy(Qt::NoFocus);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);

	QStandardItemModel* pItemModel(new QStandardItemModel(0, 7, this));
	pItemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	pItemModel->setHorizontalHeaderItem(1, new QStandardItem("Center X"));
	pItemModel->setHorizontalHeaderItem(2, new QStandardItem("Center Y"));
	pItemModel->setHorizontalHeaderItem(3, new QStandardItem("Center Z"));
	pItemModel->setHorizontalHeaderItem(4, new QStandardItem("Volume"));
	pItemModel->setHorizontalHeaderItem(5, new QStandardItem("Ball diameter"));
	pItemModel->setHorizontalHeaderItem(6, new QStandardItem("Bounding box"));

	horizontalHeader()->setSectionsClickable(false);

	verticalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

	setModel(pItemModel);
}

void iAFoamCharacterizationTableAnalysis::setRow(const int& iRow, const long& lLabel, const double& dCenterX,
	const double& dCenterY, const double& dCenterZ, const double& dVolume, const double& dDiameter,
	std::array<int64_t, 3> bbOrigin, std::array<uint64_t, 3> bbSize)
{
	m_vData[iRow].set(lLabel, dCenterX, dCenterY, dCenterZ, dVolume, dDiameter);

	QStandardItemModel* pModel ((QStandardItemModel*) model());

	const QModelIndex miValue0(pModel->index(iRow, 0));
	pModel->setData(miValue0, Qt::AlignCenter, Qt::TextAlignmentRole);
	int label = static_cast<int>(lLabel);
	pModel->setData(miValue0, label, Qt::DisplayRole);

	const QModelIndex miValue1(pModel->index(iRow, 1));
	pModel->setData(miValue1, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue1, dCenterX, Qt::DisplayRole);

	const QModelIndex miValue2(pModel->index(iRow, 2));
	pModel->setData(miValue2, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue2, dCenterY, Qt::DisplayRole);

	const QModelIndex miValue3(pModel->index(iRow, 3));
	pModel->setData(miValue3, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue3, dCenterZ, Qt::DisplayRole);

	const QModelIndex miValue4(pModel->index(iRow, 4));
	pModel->setData(miValue4, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue4, dVolume, Qt::DisplayRole);

	const QModelIndex miValue5(pModel->index(iRow, 5));
	pModel->setData(miValue5, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue5, dDiameter, Qt::DisplayRole);

	const QString sBoundingBox(QString("(%1) (%2)").arg(arrayToString(bbOrigin)).arg(arrayToString(bbSize)));

	const QModelIndex miValue6(pModel->index(iRow, 6));
	pModel->setData(miValue6, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue6, sBoundingBox, Qt::DisplayRole);

}

void iAFoamCharacterizationTableAnalysis::setRowCount(const int& _iRowCount)
{
	((QStandardItemModel*) model())->setRowCount(_iRowCount);

	m_vData.resize(_iRowCount);
}
