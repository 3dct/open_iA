// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_TFTable.h"

class iAChartFunction;
class iATransferFunction;

//! Dialog for editing the precise values of a transfer function.
class iATFTableDlg : public QDialog, public Ui_TFTableWidget
{
	Q_OBJECT

public:
	iATFTableDlg(QWidget* parent, iAChartFunction* func);

public slots:
	void updateTable();

signals:
	void transferFunctionChanged();

private slots:
	void changeColor();
	void addPoint();
	void removeSelectedPoint();
	void itemClicked(QTableWidgetItem*);
	void cellValueChanged(int, int);

private:
	bool isValueXValid(double xVal, int row = -1);
	void updateTransferFunction();

	iATransferFunction* m_tf;
	QColor m_newPointColor;
	double m_xRange[2];
	double m_oldItemValue;
};
