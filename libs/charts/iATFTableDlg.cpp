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
#include "iATFTableDlg.h"

#include "iAChartWithFunctionsWidget.h"
#include "iAChartFunctionTransfer.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QColorDialog>
#include <QMessageBox>

const QStringList columnNames = QStringList() << "X" << "Y" << "Color";

// override operator < for search ... TODO: maybe this could be done via a standalone operator?
class iATableWidgetItem : public QTableWidgetItem
{
public:
	bool operator<(const QTableWidgetItem& other) const
	{
		return text().toDouble() < other.text().toDouble();
	}
};

iATFTableDlg::iATFTableDlg(iAChartWithFunctionsWidget* parent, iAChartFunction* func) :
	iATFTableWidgetConnector(parent),
	m_tf(dynamic_cast<iAChartTransferFunction*>(func)->tf()),
	m_newPointColor(Qt::gray),
	m_parent(parent)
{
	m_tf->opacityTF()->GetRange(m_xRange);
	dsbNewPointX->setRange(m_xRange[0], m_xRange[1]);

	QPixmap pxMap(23, 23);
	pxMap.fill(m_newPointColor);
	tbChangeColor->setIcon(pxMap);

	QAction* removePnt = new QAction(tr("Remove Point"), this);
	QAction* addPnt = new QAction(tr("add Point"), this);
	QAction* updateHisto = new QAction(tr("Update Histogram"), this);
	removePnt->setShortcut(Qt::Key_Delete);
	addPnt->setShortcut(Qt::Key_Space);
	updateHisto->setShortcut(Qt::Key_Enter);
	addAction(addPnt);
	addAction(updateHisto);
	table->addAction(removePnt);
	table->setColumnCount(columnNames.size());
	table->setHorizontalHeaderLabels(columnNames);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->verticalHeader()->setDefaultSectionSize(25);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);

	connect(tbChangeColor, &QToolButton::clicked, this, &iATFTableDlg::changeColor);
	connect(tbAddPoint, &QToolButton::clicked, this, &iATFTableDlg::addPoint);
	connect(addPnt, &QAction::triggered, this, &iATFTableDlg::addPoint);
	connect(tbRemovePoint, &QToolButton::clicked, this, &iATFTableDlg::removeSelectedPoint);
	connect(removePnt, &QAction::triggered, this, &iATFTableDlg::removeSelectedPoint);
	connect(tbUpdateHisto, &QToolButton::clicked, this, &iATFTableDlg::updateHistogram);
	connect(updateHisto, &QAction::triggered, this, &iATFTableDlg::updateHistogram);
	connect(table, &QTableWidget::itemClicked, this, &iATFTableDlg::itemClicked);
	connect(table, &QTableWidget::cellChanged, this, &iATFTableDlg::cellValueChanged);

	updateTable();
	resize(table->columnWidth(0) * columnNames.size(), table->rowHeight(0) * 13);
}

void iATFTableDlg::updateTable()
{
	table->setRowCount(m_tf->opacityTF()->GetSize());
	table->blockSignals(true);
	for (int i = 0; i < m_tf->opacityTF()->GetSize(); ++i)
	{
		double pointValue[4], color[4];
		m_tf->opacityTF()->GetNodeValue(i, pointValue);
		m_tf->colorTF()->GetIndexedColor(i, color);
		QColor c;
		c.setRgbF(color[0], color[1], color[2], color[3]);
		iATableWidgetItem* xItem = new iATableWidgetItem;
		iATableWidgetItem* yItem = new iATableWidgetItem;
		iATableWidgetItem* colorItem = new iATableWidgetItem;
		xItem->setData(Qt::DisplayRole, QString::number(pointValue[0]));
		yItem->setData(Qt::DisplayRole, QString::number(pointValue[1]));
		colorItem->setBackground(c);
		if (i == 0 || i == m_tf->opacityTF()->GetSize() - 1)
		{
			xItem->setFlags(xItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
			yItem->setFlags(yItem->flags() & ~Qt::ItemIsSelectable);
			colorItem->setFlags(colorItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
		}
		table->setItem(i, 0, xItem);
		table->setItem(i, 1, yItem);
		table->setItem(i, 2, colorItem);
	}
	table->blockSignals(false);
}

void iATFTableDlg::changeColor()
{
	m_newPointColor = QColorDialog::getColor(Qt::gray, this, "Set Color", QColorDialog::ShowAlphaChannel);
	if (!m_newPointColor.isValid())
	{
		return;
	}
	QPixmap pxMap(23, 23);
	pxMap.fill(m_newPointColor);
	tbChangeColor->setIcon(pxMap);
}

void iATFTableDlg::addPoint()
{
	if (!isValueXValid(dsbNewPointX->value()))
	{
		return;
	}
	table->insertRow(table->rowCount());
	table->blockSignals(true);
	iATableWidgetItem* newXItem = new iATableWidgetItem;
	iATableWidgetItem* newYItem = new iATableWidgetItem;
	iATableWidgetItem* newColorItem = new iATableWidgetItem;
	newXItem->setData(Qt::DisplayRole, QString::number((double)dsbNewPointX->value()));
	newYItem->setData(Qt::DisplayRole, QString::number((double)dsbNewPointY->value()));
	table->setSortingEnabled(false);
	table->setItem(table->rowCount() - 1, 0, newXItem);
	table->setItem(table->rowCount() - 1, 1, newYItem);
	newColorItem->setBackground(m_newPointColor);
	table->setItem(table->rowCount() - 1, 2, newColorItem);
	table->setSortingEnabled(true);
	table->sortByColumn(0, Qt::AscendingOrder);
	table->blockSignals(false);
}

void iATFTableDlg::removeSelectedPoint()
{
	QList<QTableWidgetSelectionRange> selRangeList = table->selectedRanges();
	QList<int> rowsToRemove;
	// Bug fix: first/last row selection (despite: ~Qt::ItemIsSelectable)
	for (int i = 0; i < selRangeList.size(); ++i)
	{
		for (int j = selRangeList[i].topRow(); j <= selRangeList[i].bottomRow(); ++j)
		{
			if (j == 0 || j == table->rowCount() - 1)
			{
				continue;
			}
			rowsToRemove.append(j);
		}
	}
	std::sort(rowsToRemove.begin(), rowsToRemove.end(), std::greater<int>());
	for (int row : rowsToRemove)
	{
		table->removeRow(row);
	}
}

void iATFTableDlg::updateHistogram()
{
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();
	for (int i = 0; i < table->rowCount(); ++i)
	{
		double x = table->item(i, 0)->data(Qt::DisplayRole).toDouble();
		double y = table->item(i, 1)->data(Qt::DisplayRole).toDouble();
		QColor c = table->item(i, 2)->background().color();
		m_tf->opacityTF()->AddPoint(x, y);
		m_tf->colorTF()->AddRGBPoint(x, c.redF(), c.greenF(), c.blueF());
	}
	m_parent->update();
}

void iATFTableDlg::itemClicked(QTableWidgetItem* item)
{
	if (item->column() == 2)
	{
		table->blockSignals(true);
		QColor newItemColor = QColorDialog::getColor(Qt::gray, this, "Set Color", QColorDialog::ShowAlphaChannel);
		if (!newItemColor.isValid())
		{
			return;
		}
		item->setBackground(newItemColor);
		table->blockSignals(false);
	}
	else
	{
		m_oldItemValue = item->data(Qt::DisplayRole).toDouble();
	}
}

void iATFTableDlg::cellValueChanged(int changedRow, int changedColumn)
{
	double val = table->item(changedRow, changedColumn)->data(Qt::DisplayRole).toDouble();
	table->blockSignals(true);
	switch (changedColumn)
	{
	case 0:
		if (!isValueXValid(val, changedRow))
		{
			table->item(changedRow, changedColumn)->setData(Qt::DisplayRole, QString::number(m_oldItemValue));
		}
		break;
	case 1:
		if (val < 0.0 || val > 1.0)
		{
			table->item(changedRow, changedColumn)->setData(Qt::DisplayRole, QString::number(m_oldItemValue));
		}
		break;
	default:
		break;
	}
	table->sortByColumn(0, Qt::AscendingOrder);
	table->blockSignals(false);
}

bool iATFTableDlg::isValueXValid(double xVal, int row)
{
	if (xVal <= m_xRange[0] || xVal >= m_xRange[1])
	{
		QMessageBox::warning(this, "Transfer Function Table View: Add Point",
			"X-value out of range. Set value between transfer function start point and end point");
		return false;
	}
	for (int i = 0; i < table->rowCount(); ++i)
	{
		if (i == row)
		{
			continue;
		}
		if (xVal == table->item(i, 0)->data(Qt::DisplayRole).toDouble())
		{
			QMessageBox::warning(this, "Transfer Function Table View: Add Point",
				tr("Cannot add point. X-value already exists in row %1.").arg(i + 1));
			return false;
		}
	}
	return true;
}
