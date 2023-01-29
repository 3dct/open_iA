// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

class QDialogButtonBox;
class QLineEdit;
class QLabel;
class QString;

class iAClassEditDlg : public QDialog
{
	Q_OBJECT
public:
	static QString getClassInfo(const QString &title, const QString &text, QColor &color, bool &ok);

private:
	iAClassEditDlg();
	void setTextValue(const QString &text);
	QString getTextValue();
	void setColor(QColor const & color);
	void getColor(QColor & color);
	void getColorDialog();

	QLabel *cNameLabel, *cColorLabel;
	QLineEdit *nameEdit;
	QPushButton *colorButton;
	QDialogButtonBox *buttonBox;
	QColor dcolor;
};
