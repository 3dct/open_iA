/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAParameterDlg.h"

#include "iAAttributeDescriptor.h"
#include "iAFilterSelectionDlg.h"
#include "iALog.h"
#include "iAFilter.h"
#include "iAFilterRunnerRegistry.h"
#include "iAFilterRunnerGUI.h"
#include "iAStringHelper.h"
#include "io/iAFileChooserWidget.h"
#include "iAMdiChild.h"

#include <vtkImageData.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QErrorMessage>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextDocument>

enum ContainerSize {
	WIDTH=530, HEIGHT=600
};

namespace
{
	iAFileChooserWidget::ChoiceType mapValueTypeToFileChoiceType(iAValueType valueType)
	{
		switch (valueType)
		{
		default:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::FileNameOpen:  return iAFileChooserWidget::FileNameOpen;
		case iAValueType::FileNamesOpen: return iAFileChooserWidget::FileNamesOpen;
		case iAValueType::FileNameSave:  return iAFileChooserWidget::FileNameSave;
		case iAValueType::Folder:        return iAFileChooserWidget::Folder;
		}
	}
	iAFileChooserWidget* createFileChooser(iAValueType type, QString const & value)
	{
		auto newWidget =  new iAFileChooserWidget(nullptr, mapValueTypeToFileChoiceType(type));
		newWidget->setText(value);
		return newWidget;
	}
}

iAParameterDlg::iAParameterDlg(QWidget* parent, QString const& title, ParamListT parameters, QString const& descr)
	: QDialog(parent),
	m_sourceMdiChild(nullptr),
	m_sourceMdiChildClosed(false),
	m_widgetList(parameters.size()),
	m_parameters(parameters)
{
	auto gridLayout = new QGridLayout();
	setLayout(gridLayout);
	gridLayout->setContentsMargins(4, 4, 4, 4);
	m_buttonBox = new QDialogButtonBox();
	m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
	connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	if (title.isEmpty())
	{
		LOG(lvlError, "No window title entered. Please give a window title");
		auto lbl = new QLabel("No window title entered. Please give a window title");
		gridLayout->addWidget(lbl, 0, 0);
		m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		gridLayout->addWidget(m_buttonBox, 1, 0);
		return;
	}
	this->setWindowTitle(title);

	//Generates a scrollable container for the widgets with a grid layout
	auto scrollArea = new QScrollArea(this);

	if (!descr.isEmpty())
	{
		auto info = new QTextBrowser();
		QTextDocument *doc = new QTextDocument(info); // set info as parent so it will get deleted along with it
		doc->setHtml(descr);
		info->setDocument(doc);
		info->setReadOnly(true);
		info->setOpenExternalLinks(true);
		gridLayout->addWidget(info, 0, 0);
		// make sure that description can be easily resized; parameters have scroll bar
		scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}

	scrollArea->setObjectName("scrollArea");
	m_container = new QWidget(scrollArea);
	m_container->setObjectName("container");
	auto containerLayout = new QGridLayout(m_container);
	containerLayout->setObjectName("containerLayout");

	for (int i = 0; i < parameters.size(); ++i)
	{
		auto p = parameters[i];
		QLabel* label = new QLabel(m_container);
		label->setText(p->name());
		containerLayout->addWidget(label, i, 0);
		QWidget* newWidget = nullptr;

		switch (p->valueType())
		{
		case iAValueType::FilterParameters:
		{
			if (parameters[i - 1]->valueType() == iAValueType::FilterName)
			{                                            // if this is a filter parameter string,
				m_filterWithParameters.push_back(i - 1); // and previous was a filter name, then
			}                                            // remember this for the filter selection
			auto lineEdit = new QLineEdit(m_container);
			lineEdit->setText(p->defaultValue().toString());
			newWidget = lineEdit;
			break;
		}
		case iAValueType::Continuous:
		{
			auto lineEdit = new QLineEdit(m_container);
			lineEdit->setText(p->defaultValue().toString());
			newWidget = lineEdit;
			/* // use double spin box?
				auto doubleSpinBox = new QDoubleSpinBox(m_container);
				doubleSpinBox->setSingleStep (0.001);
				doubleSpinBox->setDecimals(6);
				doubleSpinBox->setRange(-999999, 999999);
				doubleSpinBox->setValue(values[paramIdx++].toDouble());
				newWidget = doubleSpinBox;
				break;
			*/
			break;
		}
		case iAValueType::Discrete:
		{
			auto spinBox = new QSpinBox(m_container);
			int minValue = (p->min() < std::numeric_limits<int>::lowest()) ? std::numeric_limits<int>::lowest() : static_cast<int>(p->min());
			int maxValue = (p->max() > std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : static_cast<int>(p->max());
			spinBox->setRange(minValue, maxValue);
			spinBox->setObjectName(p->name());	// required for ROI (parses object name)
			spinBox->setValue(p->defaultValue().toInt());
			newWidget = spinBox;
			break;
		}
		case iAValueType::Boolean:
		{
			auto checkBox = new QCheckBox(m_container);
			checkBox->setChecked(p->defaultValue().toBool());
			newWidget = checkBox;
			break;
		}
		case iAValueType::Categorical:
		{
			auto comboBox = new QComboBox(m_container);
			for (QString s : p->defaultValue().toStringList())
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
			newWidget = comboBox;
			break;
		}
		default:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::String:
		{
			auto textEdit = new QLineEdit(m_container);
			textEdit->setText(p->defaultValue().toString());
			newWidget = textEdit;
			break;
		}
		case iAValueType::Text:
		{
			auto plainTextEdit = new QPlainTextEdit(m_container);
			plainTextEdit->setPlainText(p->defaultValue().toString());
			newWidget = plainTextEdit;
			break;
		}
		case iAValueType::FilterName:
		{
			auto button = new QPushButton(m_container);
			button->setText(p->defaultValue().toString());
			newWidget = button;
			connect(button, &QPushButton::clicked, this, &iAParameterDlg::selectFilter);
			break;
		}
		case iAValueType::FileNameOpen:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::FileNamesOpen:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::FileNameSave:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::Folder:
		{
			newWidget = createFileChooser(p->valueType(), p->defaultValue().toString());
			break;
		}
		}
		m_widgetList[i] = newWidget;
		containerLayout->addWidget(newWidget, i, 1);
	}

	//Controls the containers width and sets the correct width for the widgets
	containerLayout->setColumnMinimumWidth(0, WIDTH/3);
	containerLayout->setColumnMinimumWidth(1, WIDTH/3);
	m_container->setLayout(containerLayout);

	//Set scrollbar if needed
	if(containerLayout->minimumSize().height() > HEIGHT)
	{
		scrollArea->setMinimumHeight(HEIGHT);
	}
	else
	{
		scrollArea->setMinimumHeight(containerLayout->minimumSize().height()+5);
	}
	if(containerLayout->minimumSize().width() > WIDTH)
	{
		scrollArea->setMinimumWidth(WIDTH+20);
	}
	else
	{
		scrollArea->setMinimumWidth(containerLayout->minimumSize().width());
	}
	scrollArea->setWidget(m_container);
	scrollArea->setWidgetResizable(true);

	// make scrollArea widgets backround transparent
	QPalette pal = scrollArea->palette();
	pal.setColor(scrollArea->backgroundRole(), Qt::transparent);
	scrollArea->setPalette(pal);

	gridLayout->addWidget(scrollArea, 1, 0);
	gridLayout->addWidget(m_buttonBox, 2, 0);
}

void  iAParameterDlg::setSourceMdi(iAMdiChild* child, iAMainWindow* mainWnd)
{
	m_sourceMdiChild = child;
	m_mainWnd = mainWnd;
	connect(child, &iAMdiChild::closed, this, &iAParameterDlg::sourceChildClosed);
}

QVector<QWidget*> iAParameterDlg::widgetList()
{
	return m_widgetList;
}

void iAParameterDlg::selectFilter()
{
	QPushButton* sender = qobject_cast<QPushButton*>(QObject::sender());
	iAFilterSelectionDlg dlg(this, sender->text());
	if (dlg.exec())
	{
		QString filterName = dlg.selectedFilterName();
		int idx = m_widgetList.indexOf(sender);
		if (idx < m_widgetList.size() - 1 && m_filterWithParameters.indexOf(idx) != -1 &&
			m_sourceMdiChild)	// TODO: if possible, get rid of sourceMdi?
		{
			auto filter = iAFilterRegistry::filter(filterName);
			int filterID = iAFilterRegistry::filterID(filterName);
			auto runner = iAFilterRunnerRegistry::filterRunner(filterID)->create();
			QMap<QString, QVariant> paramValues = runner->loadParameters(filter, m_sourceMdiChild);
			if (!runner->askForParameters(filter, paramValues, m_sourceMdiChild, m_mainWnd, false))
			{
				return;
			}
			QString paramStr;
			for (auto param: filter->parameters())
			{
				paramStr += (paramStr.isEmpty() ? "" : " ");
				switch (param->valueType())
				{
				case iAValueType::Boolean:
					paramStr += paramValues[param->name()].toBool() ? "true" : "false"; break;
				case iAValueType::Discrete:
				case iAValueType::Continuous:
					paramStr += paramValues[param->name()].toString(); break;
				default:
					paramStr += quoteString(paramValues[param->name()].toString()); break;
				}

			}
			QLineEdit* e = qobject_cast<QLineEdit*>(m_widgetList[idx + 1]);
			if (e)
			{
				e->setText(paramStr);
			}
			else
			{
				LOG(lvlError, QString("Parameter string %1 could not be set!").arg(paramStr));
			}
		}
		sender->setText(filterName);
	}
}

void iAParameterDlg::showROI()
{
	if (!m_sourceMdiChild)
	{
		LOG(lvlError, "You need to call setSourceMDI before show ROI!");
		return;
	}
	QObjectList children = m_container->children();
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
			connect(input, QOverload<int>::of(&QSpinBox::valueChanged), this, &iAParameterDlg::updatedROI);
			updateROIPart(input->objectName(), input->value());
		}
	}
	m_sourceMdiChild->setROIVisible(true);
	m_sourceMdiChild->updateROI(m_roi);
}

void iAParameterDlg::updatedROI(int value)
{
	if (m_sourceMdiChildClosed)
	{
		return;
	}
	QString senderName = QObject::sender()->objectName();
	updateROIPart(senderName, value);
	m_sourceMdiChild->updateROI(m_roi);
}

void iAParameterDlg::updateROIPart(QString const & partName, int value)
{
	if (partName.contains("Index X"))
	{
		m_roi[0] = value;
	}
	else if (partName.contains("Index Y"))
	{
		m_roi[1] = value;
	}
	else if (partName.contains("Index Z"))
	{
		m_roi[2] = value;
	}
	else if (partName.contains("Size X"))
	{
		m_roi[3] = value > 0 ? value : 0;
	}
	else if (partName.contains("Size Y"))
	{
		m_roi[4] = value > 0 ? value : 0;
	}
	else if (partName.contains("Size Z"))
	{
		m_roi[5] = value > 0 ? value : 0;
	}
}

void iAParameterDlg::sourceChildClosed()
{
	m_sourceMdiChildClosed = true;
}

QMap<QString, QVariant> iAParameterDlg::parameterValues() const
{
	QMap<QString, QVariant> result;
	QString msg;
	for (int i = 0; i < m_parameters.size(); ++i)
	{
		auto p = m_parameters[i];
		switch (p->valueType())
		{
		case iAValueType::FilterParameters:
		{
			auto t = qobject_cast<QLineEdit*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->text());
			break;
		}
		case iAValueType::Continuous:
		{
			auto t = qobject_cast<QLineEdit*>(m_widgetList[i]);
			assert(t);
			bool ok;
			double value = t->text().toDouble(&ok);
			if (!ok)
			{
				msg += "%1 is not a valid value for '%1'; please enter a valid value!\n";
			}
			result.insert(p->name(), value);
			/* // use double spin box?
			QDoubleSpinBox *t = qobject_cast<QDoubleSpinBox*>(m_widgetList[index]);
			assert(t);
			result.insert(p->name(), t->value());
			break;
			*/
			break;
		}
		case iAValueType::Discrete:
		{
			QSpinBox* t = qobject_cast<QSpinBox*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->value());
			break;
		}
		case iAValueType::Boolean:
		{
			QCheckBox* t = qobject_cast<QCheckBox*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->checkState());
			break;
		}
		case iAValueType::Categorical:
		{
			QComboBox* t = qobject_cast<QComboBox*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->currentText());
			break;
		}
		default:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::String:
		{
			QLineEdit* t = qobject_cast<QLineEdit*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->text());
			break;
		}
		case iAValueType::Text:
		{
			QPlainTextEdit* t = qobject_cast<QPlainTextEdit*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->toPlainText());
			break;
		}
		case iAValueType::FilterName:
		{
			QPushButton* t = qobject_cast<QPushButton*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->text());
			break;
		}
		case iAValueType::FileNameOpen:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::FileNamesOpen:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::FileNameSave:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
		case iAValueType::Folder:
		{
			iAFileChooserWidget* t = qobject_cast<iAFileChooserWidget*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->text());
			break;
		}
		}
	}
	return result;
}

int iAParameterDlg::exec()
{
	int result = QDialog::exec();
	if (m_sourceMdiChildClosed || (m_sourceMdiChild && !qobject_cast<QWidget*>(parent())->isVisible()))
	{
		return QDialog::Rejected;
	}
	if (m_sourceMdiChild)
	{
		disconnect(m_sourceMdiChild, &iAMdiChild::closed, this, &iAParameterDlg::sourceChildClosed);
		m_sourceMdiChild->setROIVisible(false);
	}
	return result;
}

void iAParameterDlg::setOKEnabled(bool enabled)
{
	m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void addParameter(iAParameterDlg::ParamListT params, QString const& name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	params.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
}

void selectOption(QStringList& options, QString const& selected)
{
	for (int i = 0; i < options.size(); ++i)
	{
		if (options[i].compare(selected, Qt::CaseInsensitive) == 0)
		{
			options[i] = "!" + options[i];
		}
	}
}