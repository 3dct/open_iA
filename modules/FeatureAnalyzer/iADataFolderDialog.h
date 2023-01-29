// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_dataFolderDialog.h"

class iADataFolderDialog : public QDialog, public Ui_dataFolderDialog
{
	Q_OBJECT
public:
	iADataFolderDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~iADataFolderDialog();
	QString ResultsFolderName();
	QString DatasetsFolderName();
private slots:
	void browseDataFolder();
	void browseDatasetsFolder();
	void okBtnClicked();
};
