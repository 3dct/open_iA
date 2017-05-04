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

#include <QCheckBox>
#include <QComboBox>
#include <QErrorMessage>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QScrollArea>
#include <QTextEdit>

enum ContainerSize {
	WIDTH=530, HEIGHT=600
};

dlg_commoninput::dlg_commoninput(QWidget *parent, QString winTitle, int n, QStringList inList, QList<QVariant> inPara, QTextDocument *fDescr, bool modal) : QDialog (parent)
{
	//initialize a instance of error message dialog box
	eMessage = new QErrorMessage(this);

	this->setModal(modal);

	//check whether the input parameters are correct
	//if the input parameter are not correct show error message
	//if the input parameters are correct, then initialize the dialog class
	if  (winTitle.isEmpty())
		eMessage->showMessage("No window title entered. Please give a window title");
	else if (inList.size() != n)
		eMessage->showMessage("The number of strings in the string list and the number of parameters entered does not match.");
	else
	{
		//setup the ui dialog class widget as this
		setupUi(this);
		
		NoofComboBox = 0;
		//int ComboBoxCounter = 0;
		//initialize variables
		numPara = n;

		//set the window title
		this->setWindowTitle(winTitle);

		for ( int i = 0; i < numPara; i++)
		{
			tStr = inList[i];
			if (tStr.contains("+")) 
			{
				NoofComboBox++;
			}
		}
		
		//Generates the description in the CommonInput dialog 
		if(fDescr){

			//get line count to differ between median filter and mmRegistration 
			//HZ, 3.12.2013
			int lines_count = fDescr->toPlainText().count("\n");
			
			QTextEdit* info = new QTextEdit();
			


			QPalette p = info->palette();
			p.setColor(QPalette::Base, QColor(240, 240, 255));
			info->setPalette(p);
			info->setDocument(fDescr);
			info->setReadOnly(true);
			info->setStyleSheet("QTextEdit {margin-left:10px;}");
			gridLayout->addWidget(info, 0, 0 , 1, 2);
		}

		//Generates a scrollable container for the widgets with a grid layout
		scrollArea = new QScrollArea(this);
		scrollArea->setObjectName("scrollArea");
		container = new QWidget(scrollArea);
		container->setObjectName("container");
		containerLayout = new QGridLayout(container);
		containerLayout->setObjectName("containerLayout");
	
		for ( int i = 0; i < numPara; i++)
		{
			QString tStr = inList[i];
			
			if ( !tStr.contains(QRegExp("[$#+*^?]")) )
				eMessage->showMessage(QString("Unknown widget prefix '").append(inList[i][0]).append("' for label \"").append(tStr.remove(0, 1)).append("\""));
			else
				tStr.remove(0, 1);
				
			QString tempStr = tStr;

			QLabel *label = new QLabel(container);
			label->setObjectName(tempStr.append("Label"));
			widgetList.insert(i, tempStr);		
			label->setText(tStr); 
			containerLayout->addWidget(label, i, 0, 1, 1);

			/////////////////////////////
			// $ -> switch off line edit
			// # -> switch off checkbox
			/////////////////////////////
			QWidget *newWidget;

			tempStr = tStr;
			switch(inList[i].at(0).toLatin1())
			{
				case '$':
				{
					newWidget = new QCheckBox(container);
					newWidget->setObjectName(tempStr.append("CheckBox"));
				}
				break;
				case '#':
				{
					newWidget = new QLineEdit(container);
					newWidget->setObjectName(tempStr.append("LineEdit"));
				}
				break;
				case '+':
				{
					tempStr = tStr;
					newWidget = new QComboBox(container);
					newWidget->setObjectName(tempStr.append("ComboBox"));

				}
				break;
				case '*':
				{
					tempStr = tStr;
					newWidget = new QSpinBox(container);
					newWidget->setObjectName(tempStr.append("SpinBox"));
					((QSpinBox*)newWidget)->setRange(0, 65536);
				}
				break;
				case '^':	// alexander 14.10.2012
					{
						tempStr = tStr;
						newWidget = new QDoubleSpinBox(container);
						newWidget->setObjectName(tempStr.append("QDoubleSpinBox"));
						((QDoubleSpinBox*)newWidget)->setSingleStep (0.001);
						((QDoubleSpinBox*)newWidget)->setDecimals(6);
						((QDoubleSpinBox*)newWidget)->setRange(-999999, 999999);
					}
				break;
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
		containerLayout->setColumnMinimumWidth(1, WIDTH + 30 - containerLayout->minimumSize().width());
		
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

		//add the container to the scrollarea 
		scrollArea->setWidget(container);
		
		//make scrollArea widgets backround transparent
		QPalette pal = scrollArea->palette();
		pal.setColor(scrollArea->backgroundRole(), Qt::transparent);
		scrollArea->setPalette(pal);
		
		//add the scrollarea to the gridlayout
		gridLayout->addWidget(scrollArea, 1, 0, 1, 1);

		updateValues(inPara);
		//add the ok and cancel button to the gridlayout
		gridLayout->addWidget(buttonBox, 2, 0, 1, 1);
		
	}
}

void dlg_commoninput::updateValues(QList<QVariant> inPara)
{
	QObjectList children = this->container->children();

	int paramIdx = 0;
	for ( int i = 0; i < children.size(); i++)
	{
		QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(children.at(i));
		if (lineEdit != NULL)
			lineEdit->setText(inPara[paramIdx++].toString()); 

		QComboBox *comboBox = dynamic_cast<QComboBox*>(children.at(i));
		
		if (comboBox != NULL){
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
		if (checkBox != NULL)
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
		if (spinBox != NULL)
			spinBox->setValue(inPara[paramIdx++].toDouble());

		QDoubleSpinBox *doubleSpinBox = dynamic_cast<QDoubleSpinBox*>(children.at(i));
		if (doubleSpinBox != NULL)
			doubleSpinBox->setValue(inPara[paramIdx++].toDouble());
	}
}

void dlg_commoninput::connectMdiChild(MdiChild *child)
{
	QObjectList children = this->container->children();

	for (int i = 0; i < children.size(); i++)
	{
		QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(children.at(i));
		if (lineEdit != NULL)
		{
			QString objectName = lineEdit->objectName();
			connect(lineEdit, SIGNAL(textEdited(QString)), child, SLOT(updated(QString)));
		}

		QComboBox *comboBox = dynamic_cast<QComboBox*>(children.at(i));
		
		if (comboBox != NULL){
			QString objectName = comboBox->objectName();
			connect(comboBox, SIGNAL(currentIndexChanged(QString)), child, SLOT(updated(i, QString)));
		}

		QCheckBox *checkBox = dynamic_cast<QCheckBox*>(children.at(i));
		if (checkBox != NULL)
		{
			QString objectName = checkBox->objectName();
			connect(checkBox, SIGNAL(stateChanged(int)), child, SLOT(updated(i)));
		}

		QSpinBox *spinBox = dynamic_cast<QSpinBox*>(children.at(i));
		if (spinBox != NULL)
		{
			QString objectName = spinBox->objectName();
			connect(spinBox, SIGNAL(valueChanged(QString)), child, SLOT(updated(QString)));
		}
	}
}

QList<double> dlg_commoninput::getValues()
{
	outValueList.clear();
	for (int i = 0; i < numPara; i++)
	{
		QString test = widgetList[i];
		// find the child widget with the name in the leList
		QLineEdit *t = this->container->findChild<QLineEdit*>(widgetList[i]);
		
		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outValueList.insert(i, t->text().toDouble());
		}
		else
			outValueList.insert(i, 0.0);
	}
	return (outValueList);
}

QList<double> dlg_commoninput::getSpinBoxValues()
{
	outValueList.clear();
	for (int i = 0; i < numPara; i++)
	{
		// find the child widget with the name in the leList
		QSpinBox *t = this->container->findChild<QSpinBox*>(widgetList[i]);
		
		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outValueList.insert(i,t->text().toDouble());
		}
		else
			outValueList.insert(i, 0.0);
	}
	return (outValueList);
}

QList<double> dlg_commoninput::getDoubleSpinBoxValues()
{
	outValueList.clear();
	for (int i = 0; i < numPara; i++)
	{
		// find the child widget with the name in the leList
		QDoubleSpinBox *t = this->container->findChild<QDoubleSpinBox*>(widgetList[i]);
		
		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outValueList.insert(i,t->value());
		}
		else
			outValueList.insert(i, 0.0);
	}
	return (outValueList);
}

QList<int> dlg_commoninput::getCheckValues()
{
	outCheckList.clear();
	for (int i = 0; i < numPara; i++)
	{
		// find the child widget with the name in the leList
		QCheckBox *t = this->container->findChild<QCheckBox*>(widgetList[i]);

		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outCheckList.insert(i,t->checkState());
		}
		else
			outCheckList.insert(i, 0);
	}
	return (outCheckList);
}

QStringList dlg_commoninput::getComboBoxValues()
{
	outComboValues.clear();
	for (int i = 0; i < numPara; i++)
	{
		// find the child widget with the name in the leList
		QComboBox *t = this->container->findChild<QComboBox*>(widgetList[i]);

		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outComboValues.insert(i,t->currentText());
		}
		else
			//needs to be there otherwise the list indices are incorrect!
			outComboValues.insert(i, "");
	}
	return (outComboValues);
}

QStringList dlg_commoninput::getText()
{
	outTextList.clear();
	for (int i = 0; i < numPara; i++)
	{
		// find the child widget with the name in the leList
		QLineEdit *t = this->container->findChild<QLineEdit*>(widgetList[i]);
		
		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outTextList.insert(i,t->text());
		}
		else
			outTextList.insert(i, "");
	}
	return (outTextList);
}

double dlg_commoninput::getParameterValue(QString name)
{
	if (name.contains(QRegExp("$#*")))
		name.remove(0, 1);
	
	if ( name.contains("LineEdit", Qt::CaseSensitive) )
	{
		// find the child widget with name in the objectname
		QLineEdit *l_temp = this->container->findChild<QLineEdit*>(name);

		outValue = l_temp->text().toDouble();
	}
	else if (name.contains("ComboBox", Qt::CaseSensitive) )
	{
		// find the child widget with name in the objectname
		QComboBox *t_temp = this->container->findChild<QComboBox*>(name);

		outValue = t_temp->currentText().toDouble();
	}
	else if( name.contains("CheckBox", Qt::CaseSensitive) )
	{
		// find the child widget with name in the objectname
		QCheckBox *t_temp = this->container->findChild<QCheckBox*>(name);

		outValue = t_temp->checkState();
	}
	else if( name.contains("SpinBox", Qt::CaseSensitive) )
	{
		// find the child widget with name in the objectname
		QSpinBox *t_temp = this->container->findChild<QSpinBox*>(name);

		outValue = t_temp->text().toDouble();
	}
	else outValue = 0;

	return outValue;
}

QList<int> dlg_commoninput::getComboBoxIndices()
{
	outComboIndices.clear();
	for (int i = 0; i < numPara; i++)
	{
		// find the child widget with the name in the leList
		QComboBox *t = this->container->findChild<QComboBox*>(widgetList[i]);

		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			outComboIndices.insert(i, t->currentIndex());
		}
		else
			outComboIndices.insert(i, -1);
	}
	return (outComboIndices);
}