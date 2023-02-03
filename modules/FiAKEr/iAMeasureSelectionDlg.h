// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_MeasureSelection.h"

#include <QStandardItemModel>

class iAMeasureSelectionDlg : public QDialog, public Ui_dlgMeasureSelection
{
	Q_OBJECT
public:
	using TMeasureSelection = std::vector<std::pair<int, bool>>;
	iAMeasureSelectionDlg(QWidget* parent = nullptr);
	TMeasureSelection measures() const;
	int optimizeMeasureIdx() const;
	int bestMeasureIdx() const;
private slots:
	void okBtnClicked();
private:
	QScopedPointer<QStandardItemModel> m_model;
};
