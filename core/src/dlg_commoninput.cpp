/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_commoninput.h"

#include "iAAttributeDescriptor.h"
#include "dlg_FilterSelection.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAFilterRunnerGUI.h"
#include "iAStringHelper.h"
#include "io/iAFileChooserWidget.h"
#include "mdichild.h"

#include <vtkImageData.h>

#include <QCheckBox>
#include <QComboBox>
#include <QErrorMessage>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextDocument>

enum ContainerSize {
	WIDTH=530, HEIGHT=600
};


dlg_commoninput::dlg_commoninput(QWidget *parent, QString const & winTitle, QStringList const & inList, QList<QVariant> const & inPara, QTextDocument *fDescr)
	: QDialog(parent),
	m_sourceMdiChild(nullptr),
	m_sourceMdiChildClosed(false),
	widgetList(inList.size())
{
	//initialize a instance of error message dialog box
	auto eMessage = new QErrorMessage(this);

	if (winTitle.isEmpty())
	{
		eMessage->showMessage("No window title entered. Please give a window title");
		return;
	}
	if (inList.size() != inPara.size())
	{
		eMessage->showMessage("Implementation Error: The number of of parameter descriptions and the number of given values does not match. "
			"Please report this message to the developers, along with the action you were trying to perform when it occured!");
		return;
	}
	setupUi(this);
	this->setWindowTitle(winTitle);

	if (fDescr)
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

	for ( int i = 0; i < inList.size(); ++i)
	{
		QString tStr = inList[i];
		tStr.remove(0, 1);
		QLabel *label = new QLabel(container);
		label->setText(tStr);
		containerLayout->addWidget(label, i, 0);
		QWidget *newWidget = nullptr;
		switch(inList[i].at(0).toLatin1())
		{
			case '$':
				newWidget = new QCheckBox(container);
				break;
			case '.':
				if (inList[i - 1].at(0).toLatin1() == '&')	 // if this is a filter parameter string,
					m_filterWithParameters.push_back(i - 1); // and previous was a filter name, then
				// intentional fall-through!				 // remember this for the filter selection
			case '#':
				newWidget = new QLineEdit(container);
				break;
			case '+':
				newWidget = new QComboBox(container);
				break;
			case '*':
				newWidget = new QSpinBox(container);
				((QSpinBox*)newWidget)->setRange(0, 65536);
				newWidget->setObjectName(tStr);	// required for ROI (parses object name)
				break;
			case '^':
				newWidget = new QDoubleSpinBox(container);
				((QDoubleSpinBox*)newWidget)->setSingleStep (0.001);
				((QDoubleSpinBox*)newWidget)->setDecimals(6);
				((QDoubleSpinBox*)newWidget)->setRange(-999999, 999999);
				break;
			case '=':
				newWidget = new QPlainTextEdit(container);
				break;
			case '&':
				newWidget = new QPushButton(container);
				connect(newWidget, SIGNAL(clicked()), this, SLOT(SelectFilter()));
				break;
			case '<':
				newWidget = new iAFileChooserWidget(container, iAFileChooserWidget::FileNameOpen);
				break;
			case '{':
				newWidget = new iAFileChooserWidget(container, iAFileChooserWidget::FileNamesOpen);
				break;
			case '>':
				newWidget = new iAFileChooserWidget(container, iAFileChooserWidget::FileNameSave);
				break;
			case ';':
				newWidget = new iAFileChooserWidget(container, iAFileChooserWidget::Folder);
				break;
			default:
				eMessage->showMessage(QString("Unknown widget prefix '%1' for label \"%2\"").arg(inList[i][0]).arg(tStr));
				continue;
		}
		widgetList[i] = newWidget;
		containerLayout->addWidget(newWidget, i, 1);
	}

	//Controls the containers width and sets the correct width for the widgets
	containerLayout->setColumnMinimumWidth(0, WIDTH/3);
	containerLayout->setColumnMinimumWidth(1, WIDTH/3);
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
	scrollArea->setWidgetResizable(true);

	//make scrollArea widgets backround transparent
	QPalette pal = scrollArea->palette();
	pal.setColor(scrollArea->backgroundRole(), Qt::transparent);
	scrollArea->setPalette(pal);

	gridLayout->addWidget(scrollArea, 1, 0);
	gridLayout->addWidget(buttonBox, 2, 0);//add the ok and cancel button to the gridlayout

	updateValues(inPara);
}


void  dlg_commoninput::setSourceMdi(MdiChild* child, MainWindow* mainWnd)
{
	m_sourceMdiChild = child;
	m_mainWnd = mainWnd;
	connect(child, SIGNAL(closed()), this, SLOT(SourceChildClosed()));
}


void dlg_commoninput::SelectFilter()
{
	QPushButton* sender = qobject_cast<QPushButton*>(QObject::sender());
	dlg_FilterSelection dlg(this, sender->text());
	if (dlg.exec())
	{
		QString filterName = dlg.SelectedFilterName();
		int idx = widgetList.indexOf(sender);
		if (idx < widgetList.size() - 1 && m_filterWithParameters.indexOf(idx) != -1 &&
			m_sourceMdiChild)	// TODO: if possible, get rid of sourceMdi?
		{
			auto filter = iAFilterRegistry::filter(filterName);
			int filterID = iAFilterRegistry::filterID(filterName);
			auto runner = iAFilterRegistry::filterRunner(filterID)->create();
			QMap<QString, QVariant> paramValues = runner->loadParameters(filter, m_sourceMdiChild);
			if (!runner->askForParameters(filter, paramValues, m_sourceMdiChild, m_mainWnd, false))
				return;
			QString paramStr;
			for (auto param: filter->parameters())
			{
				paramStr += (paramStr.isEmpty() ? "" : " ");
				switch (param->valueType())
				{
				case Boolean:
					paramStr += paramValues[param->name()].toBool() ? "true" : "false"; break;
				case Discrete:
				case Continuous:
					paramStr += paramValues[param->name()].toString(); break;
				default:
					paramStr += QuoteString(paramValues[param->name()].toString()); break;
				}

			}
			QLineEdit* e = qobject_cast<QLineEdit*>(widgetList[idx + 1]);
			if (e)
				e->setText(paramStr);
			else
				DEBUG_LOG(QString("Parameter string %1 could not be set!").arg(paramStr));
		}
		sender->setText(filterName);
	}
}


void dlg_commoninput::updateValues(QList<QVariant> inPara)
{
	QObjectList children = container->children();

	int paramIdx = 0;
	for ( int i = 0; i < children.size(); ++i)
	{
		QLineEdit *lineEdit = qobject_cast<QLineEdit*>(children.at(i));
		if (lineEdit)
			lineEdit->setText(inPara[paramIdx++].toString());

		QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit*>(children.at(i));
		if (plainTextEdit)
			plainTextEdit->setPlainText(inPara[paramIdx++].toString());

		QComboBox *comboBox = qobject_cast<QComboBox*>(children.at(i));
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

		QCheckBox *checkBox = qobject_cast<QCheckBox*>(children.at(i));
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

		QSpinBox *spinBox = qobject_cast<QSpinBox*>(children.at(i));
		if (spinBox)
			spinBox->setValue(inPara[paramIdx++].toDouble());

		QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(children.at(i));
		if (doubleSpinBox)
			doubleSpinBox->setValue(inPara[paramIdx++].toDouble());

		QPushButton *button = qobject_cast<QPushButton*>(children.at(i));
		if (button)
			button->setText(inPara[paramIdx++].toString());

		iAFileChooserWidget* fileChooser = qobject_cast<iAFileChooserWidget*>(children.at(i));
		if (fileChooser)
			fileChooser->setText(inPara[paramIdx++].toString());
	}
}


void dlg_commoninput::showROI()
{
	if (!m_sourceMdiChild)
	{
		DEBUG_LOG("You need to call setSourceMDI before show ROI!");
		return;
	}
	QObjectList children = container->children();
	for (int i = 0; i < 3; ++i)
	{
		m_roi[i] = 0;
		m_roi[i + 3] = m_sourceMdiChild->imagePointer()->GetDimensions()[i];
	}
	for (int i = 0; i < children.size(); ++i)
	{
		QSpinBox *input = dynamic_cast<QSpinBox*>(children.at(i));
		if (input && (input->objectName().contains("Index") || input->objectName().contains("Size")))
		{
			connect(input, SIGNAL(valueChanged(QString)), this, SLOT(ROIUpdated(QString)));
			UpdateROIPart(input->objectName(), input->text());
		}
	}
	m_sourceMdiChild->setROIVisible(true);
	m_sourceMdiChild->updateROI(m_roi);
}


void dlg_commoninput::ROIUpdated(QString text)
{
	if (m_sourceMdiChildClosed)
		return;
	QString senderName = QObject::sender()->objectName();
	UpdateROIPart(senderName, text);
	// size may not be smaller than 1 (otherwise there's a vtk error):
	if (m_roi[3] <= 0) m_roi[3] = 1;
	if (m_roi[4] <= 0) m_roi[4] = 1;
	if (m_roi[5] <= 0) m_roi[5] = 1;
	m_sourceMdiChild->updateROI(m_roi);
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


void dlg_commoninput::SourceChildClosed()
{
	m_sourceMdiChildClosed = true;
}


int dlg_commoninput::getIntValue(int index) const
{
	if (index < 0 || index >= widgetList.size())
	{
		DEBUG_LOG(QString("dlg_commoninput::getIntValue: index=%1 out of bounds(0..%2").arg(index).arg(widgetList.size() - 1));
		return 0;
	}
	QSpinBox *t = qobject_cast<QSpinBox*>(widgetList[index]);
	if (t)
		return t->value();
	QLineEdit *t2 = qobject_cast<QLineEdit*>(widgetList[index]);
	if (t2)
		return t2->text().toInt();
	DEBUG_LOG(QString("dlg_commoninput::getIntValue(%1) Not a SpinBox/ LineEdit!").arg(index));
	return 0;
}


double dlg_commoninput::getDblValue(int index) const
{
	if (index < 0 || index >= widgetList.size())
	{
		DEBUG_LOG(QString("dlg_commoninput::getDblValue: index=%1 out of bounds(0..%2").arg(index).arg(widgetList.size() - 1));
		return 0.0;
	}
	QDoubleSpinBox *t = qobject_cast<QDoubleSpinBox*>(widgetList[index]);
	if (t)
		return t->value();
	QLineEdit *t2 = qobject_cast<QLineEdit*>(widgetList[index]);
	if (t2)
		return t2->text().toDouble();
	DEBUG_LOG(QString("dlg_commoninput::getDblValue(%1) Not a Double SpinBox / LineEdit!").arg(index));
	return 0.0;
}


int dlg_commoninput::getCheckValue(int index) const
{
	if (index < 0 || index >= widgetList.size())
	{
		DEBUG_LOG(QString("dlg_commoninput::getCheckValue: index=%1 out of bounds(0..%2").arg(index).arg(widgetList.size() - 1));
		return false;
	}
	QCheckBox *t = qobject_cast<QCheckBox*>(widgetList[index]);
	if (t)
		return t->checkState();
	DEBUG_LOG(QString("dlg_commoninput::getCheckValue(%1) Not a CheckBox!").arg(index));
	return false;
}


QString dlg_commoninput::getComboBoxValue(int index) const
{
	if (index < 0 || index >= widgetList.size())
	{
		DEBUG_LOG(QString("dlg_commoninput::getComboBoxValue: index=%1 out of bounds(0..%2").arg(index).arg(widgetList.size() - 1));
		return QString();
	}
	QComboBox *t = qobject_cast<QComboBox*>(widgetList[index]);
	if (t)
		return t->currentText();
	DEBUG_LOG(QString("dlg_commoninput::getComboBoxValue(%1) Not a ComboBox!").arg(index));
	return QString();
}


int dlg_commoninput::getComboBoxIndex(int index) const
{
	QComboBox *t = qobject_cast<QComboBox*>(widgetList[index]);
	if (t)
		return t->currentIndex();
	DEBUG_LOG(QString("dlg_commoninput::getComboBoxIndex(%1) Not a ComboBox!").arg(index));
	return -1;
}


QString dlg_commoninput::getText(int index) const
{
	if (index < 0 || index >= widgetList.size())
	{
		DEBUG_LOG(QString("dlg_commoninput::getText: index=%1 out of bounds(0..%2").arg(index).arg(widgetList.size() - 1));
		return QString();
	}
	QLineEdit *t = qobject_cast<QLineEdit*>(widgetList[index]);
	if (t)
		return t->text();
	QPlainTextEdit *t2 = qobject_cast<QPlainTextEdit*>(widgetList[index]);
	if (t2)
		return t2->toPlainText();
	QPushButton * t3 = qobject_cast<QPushButton*>(widgetList[index]);
	if (t3)
		return t3->text();
	iAFileChooserWidget* t4 = qobject_cast<iAFileChooserWidget*>(widgetList[index]);
	if (t4)
		return t4->text();

	QComboBox *t5 = qobject_cast<QComboBox*>(widgetList[index]);
	if (t5)
		return t5->currentText();

	DEBUG_LOG(QString("dlg_commoninput::getText(%1) called on value which is no text!").arg(index));
	return QString();
}


int dlg_commoninput::exec()
{
	int result = QDialog::exec();
	if (m_sourceMdiChildClosed || (m_sourceMdiChild && !qobject_cast<QWidget*>(parent())->isVisible()))
		return QDialog::Rejected;
	if (m_sourceMdiChild)
	{
		disconnect(m_sourceMdiChild, SIGNAL(closed()), this, SLOT(SourceChildClosed()));
		m_sourceMdiChild->setROIVisible(false);
	}
	return result;
}
