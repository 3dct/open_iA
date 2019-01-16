/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
