/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAClassEditDlg.h"

#include <QColorDialog>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QtGui>
#include <QVBoxLayout>


iAClassEditDlg::iAClassEditDlg() : QDialog()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	cNameLabel = new QLabel;
	cNameLabel->setText("Class Name:");
	nameEdit = new QLineEdit;

	colorButton = new QPushButton;
	colorButton->setText("Set Color:");
	cColorLabel = new QLabel;

	QWidget *page = new QWidget;
	QGridLayout *layout = new QGridLayout(page);
	layout->addWidget(cNameLabel, 0, 0);
	layout->addWidget(nameEdit, 0, 1);
	layout->addWidget(colorButton, 1, 0);
	layout->addWidget(cColorLabel, 1, 1);

	mainLayout->addWidget(page);
	mainLayout->addWidget(buttonBox);

	connect(colorButton, &QPushButton::clicked, this, &iAClassEditDlg::getColorDialog);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &iAClassEditDlg::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &iAClassEditDlg::reject);
}

QString iAClassEditDlg::getClassInfo(const QString& title, const QString& text, QColor& color, bool& ok)
{
	iAClassEditDlg dialog;
	dialog.setWindowTitle(title);
	dialog.setTextValue(text);
	dialog.setColor(color);

	ok = dialog.exec() == QDialog::Accepted;
	if (ok)
	{
		dialog.getColor(color);
		return dialog.getTextValue();
	}
	else
	{
		return QString();
	}
}

void iAClassEditDlg::setTextValue(const QString& text)
{
	nameEdit->setText(text);
}

QString iAClassEditDlg::getTextValue()
{
	return nameEdit->text();
}

void iAClassEditDlg::setColor(QColor const& color)
{
	dcolor.setRgba(color.rgba());
	cColorLabel->setAutoFillBackground(true);
	QString str = QString("QLabel {background-color: rgba(%1, %2, %3, %4); }")
		.arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
	cColorLabel->setStyleSheet(str);
}

void iAClassEditDlg::getColor(QColor& color)
{
	color.setRgba(dcolor.rgba());
}

void iAClassEditDlg::getColorDialog()
{
	QColor gcolor = QColorDialog::getColor(dcolor, this, "Set Color", QColorDialog::ShowAlphaChannel);
	setColor(gcolor);
}
