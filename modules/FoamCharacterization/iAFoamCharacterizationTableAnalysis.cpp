// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationTableAnalysis.h"

#include <QHeaderView>
#include <QStandardItemModel>

iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::CTableAnalysisRow()
{
	std::fill(m_pBoundingBox, m_pBoundingBox + 6, 0.0);
}

void iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::set(const long& _lLabel
	, const double& _dCenterX, const double& _dCenterY, const double& _dCenterZ
	, const double& _dVolume, const double& _dDiameter
	, const itk::FixedArray<itk::Index<3>::IndexValueType, 6> _faBoundingBox)
{
	m_lLabel = _lLabel;

	m_dCenterX = _dCenterX;
	m_dCenterY = _dCenterY;
	m_dCenterZ = _dCenterZ;

	m_dVolume = _dVolume;
	m_dDiameter = _dDiameter;

	for (int i(0); i < 6; ++i)
	{
		m_pBoundingBox[i] = _faBoundingBox[i];
	}
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

double* iAFoamCharacterizationTableAnalysis::CTableAnalysisRow::boundingBox()
{
	return m_pBoundingBox;
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

void iAFoamCharacterizationTableAnalysis::setRow ( const int& _iRow
												 , const long& _lLabel
	                                             , const double& _dCenterX, const double& _dCenterY, const double& _dCenterZ
												 , const double& _dVolume, const double& _dDiameter
												 , const itk::FixedArray<itk::Index<3>::IndexValueType, 6> _faBoundingBox
												 )
{
	m_vData[_iRow].set(_lLabel, _dCenterX, _dCenterY, _dCenterZ, _dVolume, _dDiameter, _faBoundingBox);

	QStandardItemModel* pModel ((QStandardItemModel*) model());

	const QModelIndex miValue0(pModel->index(_iRow, 0));
	pModel->setData(miValue0, Qt::AlignCenter, Qt::TextAlignmentRole);
	int label = static_cast<int>(_lLabel);
	pModel->setData(miValue0, label, Qt::DisplayRole);

	const QModelIndex miValue1(pModel->index(_iRow, 1));
	pModel->setData(miValue1, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue1, _dCenterX, Qt::DisplayRole);

	const QModelIndex miValue2(pModel->index(_iRow, 2));
	pModel->setData(miValue2, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue2, _dCenterY, Qt::DisplayRole);

	const QModelIndex miValue3(pModel->index(_iRow, 3));
	pModel->setData(miValue3, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue3, _dCenterZ, Qt::DisplayRole);

	const QModelIndex miValue4(pModel->index(_iRow, 4));
	pModel->setData(miValue4, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue4, _dVolume, Qt::DisplayRole);

	const QModelIndex miValue5(pModel->index(_iRow, 5));
	pModel->setData(miValue5, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue5, _dDiameter, Qt::DisplayRole);

	const QString sBoundingBox ( QString("(%1, %3, %5) (%2, %4, %6)").arg(_faBoundingBox[0])
		                                                             .arg(_faBoundingBox[1])
									 								 .arg(_faBoundingBox[2])
									 								 .arg(_faBoundingBox[3])
									 								 .arg(_faBoundingBox[4])
																	 .arg(_faBoundingBox[5])
							   );

	const QModelIndex miValue6(pModel->index(_iRow, 6));
	pModel->setData(miValue6, Qt::AlignCenter, Qt::TextAlignmentRole);
	pModel->setData(miValue6, sBoundingBox, Qt::DisplayRole);

}

void iAFoamCharacterizationTableAnalysis::setRowCount(const int& _iRowCount)
{
	((QStandardItemModel*) model())->setRowCount(_iRowCount);

	m_vData.resize(_iRowCount);
}
