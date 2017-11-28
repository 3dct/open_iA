/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"
#include "dlg_commoninput.h"

#include "mdichild.h"

#include <vtkImageData.h>

#include <QCheckBox>
#include <QComboBox>
#include <QErrorMessage>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QScrollArea>
#include <QTextBrowser>
#include <QPlainTextEdit>

enum ContainerSize {
	WIDTH=530, HEIGHT=600
};


dlg_commoninput::dlg_commoninput(QWidget *parent, QString winTitle, QStringList inList, QList<QVariant> inPara, QTextDocument *fDescr)
	: QDialog (parent),
	m_roiMdiChild(nullptr),
	m_roiMdiChildClosed(false)
{
	//initialize a instance of error message dialog box
	auto eMessage = new QErrorMessage(this);
	setModal(true);

	if (winTitle.isEmpty())
	{
		eMessage->showMessage("No window title entered. Please give a window title");
		return;
	}
	if (inList.size() != inPara.size())
	{
		eMessage->showMessage("Implementation Error: The number of of parameter descriptions and the number of given values does not match. Please report this to the developers!");
		return;
	}
	setupUi(this);
	this->setWindowTitle(winTitle);

	if(fDescr)
	{
		auto info = new QTextBrowser();
		QPalette p = info->palette();
		p.setColor(QPalette::Base, QColor(240, 240, 255));
		info->setPalette(p);
		info->setDocument(fDescr);
		info->setReadOnly(true);
		info->setOpenExternalLinks(true);
		gridLayout->addWidget(info, 0, 0);
	}

	//Generates a scrollable container for the widgets with a grid layout
	auto scrollArea = new QScrollArea(this);
	scrollArea->setObjectName("scrollArea");
	container = new QWidget(scrollArea);
	container->setObjectName("container");
	auto containerLayout = new QGridLayout(container);
	containerLayout->setObjectName("containerLayout");
	
	for ( int i = 0; i < inList.size(); i++)
	{
		QString tStr = inList[i];
			
		if ( !tStr.contains(QRegExp("[$#+*^?=]")) )
			eMessage->showMessage(QString("Unknown widget prefix '").append(inList[i][0]).append("' for label \"").append(tStr.remove(0, 1)).append("\""));
		else
			tStr.remove(0, 1);

		QString tempStr = tStr;
		QLabel *label = new QLabel(container);
		label->setObjectName(tempStr.append("Label"));
		widgetList.insert(i, tempStr);
		label->setText(tStr);
		containerLayout->addWidget(label, i, 0, 1, 1);

		QWidget *newWidget;
		tempStr = tStr;
		switch(inList[i].at(0).toLatin1())
		{
			case '$':
			{
				newWidget = new QCheckBox(container);
				newWidget->setObjectName(tempStr.append("CheckBox"));
				break;
			}
			case '#':
			{
				newWidget = new QLineEdit(container);
				newWidget->setObjectName(tempStr.append("LineEdit"));
				break;
			}
			case '+':
			{
				newWidget = new QComboBox(container);
				newWidget->setObjectName(tempStr.append("ComboBox"));
				break;

			}
			case '*':
			{
				newWidget = new QSpinBox(container);
				newWidget->setObjectName(tempStr.append("SpinBox"));
				((QSpinBox*)newWidget)->setRange(0, 65536);
				break;
			}
			case '^':
			{
				newWidget = new QDoubleSpinBox(container);
				newWidget->setObjectName(tempStr.append("QDoubleSpinBox"));
				((QDoubleSpinBox*)newWidget)->setSingleStep (0.001);
				((QDoubleSpinBox*)newWidget)->setDecimals(6);
				((QDoubleSpinBox*)newWidget)->setRange(-999999, 999999);
				break;
			}
			case '=':
			{
				newWidget = new QPlainTextEdit(container);
				newWidget->setObjectName(tempStr.append("PlainTextEdit"));
				break;
			}
			case '?':
			{
				label->setStyleSheet("background-color : lightGray");
				QFont font = label->font();
				font.setBold(true);
				font.setPointSize(11);
				label->setFont(font);
				continue;
			}
			break;
		}
		widgetList.insert(i, tempStr);
		containerLayout->addWidget(newWidget, i, 1, 1, 1);
	}
		
	//Controls the containers width and sets the correct width for the widgets
	containerLayout->setColumnMinimumWidth(0, WIDTH/3);
	containerLayout->setColumnMinimumWidth(1, WIDTH/3);
	container->setMaximumWidth(WIDTH);
	container->setLayout(containerLayout);

	//Set scrollbar if needed
	if(containerLayout->minimumSize().height() > HEIGHT){
		scrollArea->setMinimumHeight(HEIGHT);
	}else{
		scrollArea->setMinimumHeight(containerLayout->minimumSize().height()+5);
	}
	if(containerLayout->minimumSize().width() > WIDTH){
		scrollArea->setMinimumWidth(WIDTH+20);
	}else{
		scrollArea->setMinimumWidth(containerLayout->minimumSize().width());
	}

	scrollArea->setWidget(container);

	//make scrollArea widgets backround transparent
	QPalette pal = scrollArea->palette();
	pal.setColor(scrollArea->backgroundRole(), Qt::transparent);
	scrollArea->setPalette(pal);

	gridLayout->addWidget(scrollArea, 1, 0);
	gridLayout->addWidget(buttonBox, 2, 0);//add the ok and cancel button to the gridlayout

	updateValues(inPara);
}


void dlg_commoninput::updateValues(QList<QVariant> inPara)
{
	QObjectList children = container->children();

	int paramIdx = 0;
	for ( int i = 0; i < children.size(); i++)
	{
		QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(children.at(i));
		if (lineEdit)
			lineEdit->setText(inPara[paramIdx++].toString());

		QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit*>(children.at(i));
		if (plainTextEdit)
			plainTextEdit->setPlainText(inPara[paramIdx++].toString());

		QComboBox *comboBox = dynamic_cast<QComboBox*>(children.at(i));
		if (comboBox)
		{
			for (QString s : inPara[paramIdx++].toStringList())
			{
				bool select = false;
				if (s.startsWith("!"))
				{
					s = s.right(s.length() - 1);
					select = true;
				}
				comboBox->addItem(s);
				if (select)
				{
					comboBox->setCurrentIndex(comboBox->count() - 1);
				}
			}
		}

		QCheckBox *checkBox = dynamic_cast<QCheckBox*>(children.at(i));
		if (checkBox)
		{
			if (inPara[paramIdx] == tr("true"))
				checkBox->setChecked(true);
			else if (inPara[paramIdx] == tr("false"))
				checkBox->setChecked(false);
			else
				checkBox->setChecked(inPara[paramIdx]!=0);
			paramIdx++;

		}

		QSpinBox *spinBox = dynamic_cast<QSpinBox*>(children.at(i));
		if (spinBox)
			spinBox->setValue(inPara[paramIdx++].toDouble());

		QDoubleSpinBox *doubleSpinBox = dynamic_cast<QDoubleSpinBox*>(children.at(i));
		if (doubleSpinBox)
			doubleSpinBox->setValue(inPara[paramIdx++].toDouble());
	}
}


void dlg_commoninput::showROI(MdiChild *child)
{
	m_roiMdiChild = child;
	setModal(false);
	hide();	show(); // required to apply change in modality!
	connect(child, SIGNAL(closed()), this, SLOT(ROIChildClosed()));
	QObjectList children = container->children();
	for (int i = 0; i < 3; ++i)
	{
		m_roi[i] = 0;
		m_roi[i + 3] = child->getImagePointer()->GetDimensions()[i];
	}
	for (int i = 0; i < children.size(); i++)
	{
		QSpinBox *lineEdit = dynamic_cast<QSpinBox*>(children.at(i));
		if (lineEdit && (lineEdit->objectName().contains("Index") || lineEdit->objectName().contains("Size")))
		{
			connect(lineEdit, SIGNAL(valueChanged(QString)), this, SLOT(ROIUpdated(QString)));
			UpdateROIPart(lineEdit->objectName(), lineEdit->text());
		}
	}
	m_roiMdiChild->SetROIVisible(true);
	m_roiMdiChild->UpdateROI(m_roi);
}


void dlg_commoninput::ROIUpdated(QString text)
{
	if (m_roiMdiChildClosed)
		return;
	QString senderName = QObject::sender()->objectName();
	UpdateROIPart(senderName, text);
	// size may not be smaller than 1 (otherwise there's a vtk error):
	if (m_roi[3] <= 0) m_roi[3] = 1;
	if (m_roi[4] <= 0) m_roi[4] = 1;
	if (m_roi[5] <= 0) m_roi[5] = 1;
	m_roiMdiChild->UpdateROI(m_roi);
}


void dlg_commoninput::UpdateROIPart(QString const & partName, QString const & value)
{
	if (partName.contains("Index X"))
		m_roi[0] = value.toInt();
	else if (partName.contains("Index Y"))
		m_roi[1] = value.toInt();
	else if (partName.contains("Index Z"))
		m_roi[2] = value.toInt();
	else if (partName.contains("Size X"))
		m_roi[3] = value.toInt();
	else if (partName.contains("Size Y"))
		m_roi[4] = value.toInt();
	else if (partName.contains("Size Z"))
		m_roi[5] = value.toInt();
}


void dlg_commoninput::ROIChildClosed()
{
	m_roiMdiChildClosed = true;
}


int dlg_commoninput::getIntValue(int index) const
{
	QSpinBox *t = container->findChild<QSpinBox*>(widgetList[index]);
	if (t)
		return t->value();
	QLineEdit *t2 = container->findChild<QLineEdit*>(widgetList[index]);
	return t2 ? t2->text().toInt() : 0;
}


double dlg_commoninput::getDblValue(int index) const
{
	QDoubleSpinBox *t = container->findChild<QDoubleSpinBox*>(widgetList[index]);
	if (t)
		return t->value();
	QLineEdit *t2 = container->findChild<QLineEdit*>(widgetList[index]);
	return t2 ? t2->text().toDouble() : 0.0;
}


int dlg_commoninput::getCheckValue(int index) const
{
	QCheckBox *t = container->findChild<QCheckBox*>(widgetList[index]);
	return t? t->checkState(): 0;
}


QString dlg_commoninput::getComboBoxValue(int index) const
{
	QComboBox *t = container->findChild<QComboBox*>(widgetList[index]);
	return t? t->currentText(): QString();
}


int dlg_commoninput::getComboBoxIndex(int index) const
{
	QComboBox *t = container->findChild<QComboBox*>(widgetList[index]);
	return t ? t->currentIndex() : -1;
}


QString dlg_commoninput::getText(int index) const
{
	QLineEdit *t = container->findChild<QLineEdit*>(widgetList[index]);
	if (t)
		return t->text();
	QPlainTextEdit *t2 = container->findChild<QPlainTextEdit*>(widgetList[index]);
	return t2 ? t2->toPlainText() : "";
}


int dlg_commoninput::exec()
{
	int result = QDialog::exec();
	if (m_roiMdiChildClosed || (m_roiMdiChild && !qobject_cast<QWidget*>(parent())->isVisible()))
		return QDialog::Rejected;
	if (m_roiMdiChild)
		m_roiMdiChild->SetROIVisible(false);
	return result;
}
