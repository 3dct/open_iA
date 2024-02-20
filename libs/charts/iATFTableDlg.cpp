// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATFTableDlg.h"

#include "iAChartFunctionTransfer.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QAction>
#include <QColorDialog>
#include <QItemDelegate>
#include <QPainter>
#include <QMessageBox>

const QStringList columnNames = QStringList() << "X" << "Y" << "Color";

//! An item in the transfer function table, overrides operator< for search.
//! @todo maybe this could be done via a standalone operator?
class iATFTableWidgetItem : public QTableWidgetItem
{
public:
	bool operator<(const QTableWidgetItem& other) const
	{
		return text().toDouble() < other.text().toDouble();
	}
};

//! Delegate for color column of transfer function table.
//! Necessary to unconditionally draw item in the specified color, no matter whether row is selected or not
class iATFColorColumnDelegate : public QItemDelegate
{
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QColor c(index.data(Qt::DisplayRole).toString());
		painter->fillRect(option.rect, c);
	}
};


iATFTableDlg::iATFTableDlg(QWidget* parent, iAChartFunction* func) :
	QDialog(parent),
	m_tf(dynamic_cast<iAChartTransferFunction*>(func)->tf()),
	m_newPointColor(Qt::gray)
{
	setupUi(this);
	m_tf->opacityTF()->GetRange(m_xRange);
	dsbNewPointX->setRange(m_xRange[0], m_xRange[1]);
	dsbNewPointX->setValue((m_xRange[0] + m_xRange[1]) / 2);

	QPixmap pxMap(23, 23);
	pxMap.fill(m_newPointColor);
	tbChangeColor->setIcon(pxMap);

	QAction* removePnt = new QAction(tr("Remove Point"), this);
	QAction* addPnt = new QAction(tr("add Point"), this);
	removePnt->setShortcut(Qt::Key_Delete);
	addPnt->setShortcut(Qt::Key_Space);
	addAction(addPnt);
	table->addAction(removePnt);
	table->setColumnCount(columnNames.size());
	table->setHorizontalHeaderLabels(columnNames);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->verticalHeader()->setDefaultSectionSize(25);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setItemDelegateForColumn(2, new iATFColorColumnDelegate);

	connect(tbChangeColor, &QToolButton::clicked, this, &iATFTableDlg::changeColor);
	connect(tbAddPoint, &QToolButton::clicked, this, &iATFTableDlg::addPoint);
	connect(addPnt, &QAction::triggered, this, &iATFTableDlg::addPoint);
	connect(tbRemovePoint, &QToolButton::clicked, this, &iATFTableDlg::removeSelectedPoint);
	connect(removePnt, &QAction::triggered, this, &iATFTableDlg::removeSelectedPoint);
	connect(table, &QTableWidget::itemClicked, this, &iATFTableDlg::itemClicked);
	connect(table, &QTableWidget::cellChanged, this, &iATFTableDlg::cellValueChanged);

	updateTable();
	resize(table->columnWidth(0) * columnNames.size(), table->rowHeight(0) * 13);
}

void iATFTableDlg::updateTable()
{
	QSignalBlocker b(table);
	table->setRowCount(m_tf->opacityTF()->GetSize());
	for (int i = 0; i < m_tf->opacityTF()->GetSize(); ++i)
	{
		double pointValue[4], color[4];
		m_tf->opacityTF()->GetNodeValue(i, pointValue);
		m_tf->colorTF()->GetIndexedColor(i, color);
		QColor c;
		c.setRgbF(color[0], color[1], color[2], color[3]);
		auto xItem     = new iATFTableWidgetItem;
		auto yItem     = new iATFTableWidgetItem;
		auto colorItem = new iATFTableWidgetItem;
		xItem->setData(Qt::DisplayRole, QString::number(pointValue[0]));
		yItem->setData(Qt::DisplayRole, QString::number(pointValue[1]));
		colorItem->setData(Qt::DisplayRole, c.name());
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
	updateTransferFunction();
}

void iATFTableDlg::addPoint()
{
	if (!isValueXValid(dsbNewPointX->value()))
	{
		return;
	}
	table->insertRow(table->rowCount());
	QSignalBlocker b(table);
	auto newXItem     = new iATFTableWidgetItem;
	auto newYItem     = new iATFTableWidgetItem;
	auto newColorItem = new iATFTableWidgetItem;
	newXItem->setData(Qt::DisplayRole, QString::number((double)dsbNewPointX->value()));
	newYItem->setData(Qt::DisplayRole, QString::number((double)dsbNewPointY->value()));
	table->setSortingEnabled(false);
	table->setItem(table->rowCount() - 1, 0, newXItem);
	table->setItem(table->rowCount() - 1, 1, newYItem);
	newColorItem->setData(Qt::DisplayRole, m_newPointColor.name());
	table->setItem(table->rowCount() - 1, 2, newColorItem);
	table->setSortingEnabled(true);
	table->sortByColumn(0, Qt::AscendingOrder);
	updateTransferFunction();
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
	updateTransferFunction();
}

void iATFTableDlg::updateTransferFunction()
{
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();
	for (int i = 0; i < table->rowCount(); ++i)
	{
		double x = table->item(i, 0)->data(Qt::DisplayRole).toDouble();
		double y = table->item(i, 1)->data(Qt::DisplayRole).toDouble();
		QColor c = QColor(table->item(i, 2)->data(Qt::DisplayRole).toString());
		m_tf->opacityTF()->AddPoint(x, y);
		m_tf->colorTF()->AddRGBPoint(x, c.redF(), c.greenF(), c.blueF());
	}
	emit transferFunctionChanged();
}

void iATFTableDlg::itemClicked(QTableWidgetItem* item)
{
	if (item->column() == 2)
	{
		QSignalBlocker b(table);
		QColor newItemColor = QColorDialog::getColor(Qt::gray, this, "Set Color", QColorDialog::ShowAlphaChannel);
		if (newItemColor.isValid())
		{
			item->setData(Qt::DisplayRole, newItemColor.name());
			updateTransferFunction();
		}
	}
	else
	{
		m_oldItemValue = item->data(Qt::DisplayRole).toDouble();
	}
}

void iATFTableDlg::cellValueChanged(int changedRow, int changedColumn)
{
	double val = table->item(changedRow, changedColumn)->data(Qt::DisplayRole).toDouble();
	QSignalBlocker b(table);
	if ( (changedColumn == 0 && !isValueXValid(val, changedRow)) ||
		 (changedColumn == 1 && (val < 0.0 || val > 1.0)) )
	{
		table->item(changedRow, changedColumn)->setData(Qt::DisplayRole, QString::number(m_oldItemValue));
	}
	else
	{
		table->sortByColumn(0, Qt::AscendingOrder);
		updateTransferFunction();
	}
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
