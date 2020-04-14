/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "dlg_editPCClass.h"

#include <QColorDialog>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QtGui>
#include <QVBoxLayout>


dlg_editPCClass::dlg_editPCClass(QWidget *parent) : QDialog(parent)
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

	this->setupConnections();
}


dlg_editPCClass::~dlg_editPCClass()
{

}

//void dlg_editPCClass::initComboBox()
//{
//	for(int i=0; i<colorNames.size(); ++i)
//	{
//		QColor color(colorNames[i]);
//		colorEdit->insertItem(i, colorNames[i]);
//		// or using Qt::DecorationRole
//		colorEdit->setItemData(i, color, Qt::BackgroundColorRole);
//	}
//}

void dlg_editPCClass::setupConnections()
{
	connect(colorButton, SIGNAL(clicked()), this, SLOT(getColorDialog()));
	connect(nameEdit, SIGNAL(textChanged(QString)), this, SLOT(notifyTextChanged()));

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

QString dlg_editPCClass::getClassInfo(QWidget *parent, const QString &title, const QString &text, QColor *color, bool *ok)
{
	dlg_editPCClass dialog(parent);
	dialog.setWindowTitle(title);
	dialog.setTextValue(text);
	dialog.setColor(color);

	int ret = dialog.exec();
	if(ok)
		*ok = !!ret;
	if(ret)
	{
		dialog.getColor(color);
		return QString ("%1,%2,%3,%4").arg(dialog.getTextValue()).
											arg(dialog.dcolor.red()).
											arg(dialog.dcolor.green()).
											arg(dialog.dcolor.blue());
	}
	else
	{
		return QString();
	}
}

void dlg_editPCClass::setTextValue(const QString &text)
{
	nameEdit->setText(text);
}

QString dlg_editPCClass::getTextValue()
{
	return nameEdit->text();
}

void dlg_editPCClass::setColor(QColor *color)
{
	this->dcolor.setRgba(color->rgba());
	cColorLabel->setAutoFillBackground(true);
	QString str = QString("QLabel {background-color: rgba(%1, %2, %3, %4); }")
		.arg(color->red()).arg(color->green()).arg(color->blue()).arg(color->alpha());
	cColorLabel->setStyleSheet(str);
}

void dlg_editPCClass::getColor(QColor *color)
{
	color->setRgba(dcolor.rgba());
}

void dlg_editPCClass::getColorDialog()
{
	QColor gcolor = QColorDialog::getColor(this->dcolor, this, "Set Color", QColorDialog::ShowAlphaChannel);
	this->setColor(&gcolor);
}

void dlg_editPCClass::notifyTextChanged()
{
	// catch empty text input
}
