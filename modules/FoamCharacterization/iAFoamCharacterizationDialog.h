// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QGroupBox;
class QLineEdit;

class iAFoamCharacterizationItem;

class iAFoamCharacterizationDialog : public QDialog
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationDialog(iAFoamCharacterizationItem* _pItem, QWidget* _pParent = nullptr);
	virtual ~iAFoamCharacterizationDialog();

private:
	QGroupBox* m_pGroupBox1 = nullptr;
	QCheckBox* m_pCheckBoxEnabled = nullptr;
	QDialogButtonBox* m_pDialogButtonBox = nullptr;
	QLineEdit* m_pLineEdit1 = nullptr;

private slots:
	void slotPushButtonCancel();

protected:
	QGroupBox* m_pGroupBox2 = nullptr;
	iAFoamCharacterizationItem* m_pItem = nullptr;
	void setLayout();

protected slots:
	virtual void slotPushButtonOk() = 0;

};
