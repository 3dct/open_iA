// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAParameterDlg.h"

#include "iAAttributeDescriptor.h"
#include "iAColorInput.h"
#include "iAFileChooserWidget.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iAFilterRunnerRegistry.h"
#include "iAFilterRunnerGUI.h"
#include "iAFilterSelectionDlg.h"
#include "iALog.h"
#include "iAMdiChild.h"
#include "iAStringHelper.h"
#include "iAVectorInput.h"

#include <vtkImageData.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
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
#if __cpluspuls >= 201703L
			[[fallthrough]];
#endif
			// fall through
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

iAParameterDlg::iAParameterDlg(QWidget* parent, QString const& title, iAAttributes const & parameters, QString const& descr)
	: QDialog(parent),
	m_sourceMdiChild(nullptr),
	m_sourceMdiChildClosed(false),
	m_widgetList(parameters.size()),
	m_parameters(parameters)
{
	auto gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(4, 4, 4, 4);
	m_buttonBox = new QDialogButtonBox();
	m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
	connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	if (title.isEmpty())
	{
		LOG(lvlError, "DEVELOPER ERROR: No window title entered. Please give a window title");
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
		// make sure that description can be easily resized; parameters have scroll bar
		scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

		auto s = new QSplitter(Qt::Vertical);
		s->addWidget(info);
		auto w = new QWidget();
		w->setLayout(gridLayout);
		s->addWidget(w);
		s->setCollapsible(0, true);
		s->setCollapsible(1, false);
		setLayout(new QHBoxLayout);
		layout()->addWidget(s);
		layout()->setContentsMargins(4, 4, 4, 4);
	}
	else
	{
		setLayout(gridLayout);
	}

	scrollArea->setObjectName("scrollArea");
	m_container = new QWidget(scrollArea);
	m_container->setObjectName("container");
	auto containerLayout = new QGridLayout(m_container);
	containerLayout->setObjectName("containerLayout");

	for (qsizetype i = 0; i < parameters.size(); ++i)
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
			[[fallthrough]];
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
		case iAValueType::FileNameOpen:  [[fallthrough]];
		case iAValueType::FileNamesOpen: [[fallthrough]];
		case iAValueType::FileNameSave:  [[fallthrough]];
		case iAValueType::Folder:
		{
			newWidget = createFileChooser(p->valueType(), p->defaultValue().toString());
			break;
		}
		case iAValueType::Color:
		{
			newWidget = new iAColorInput(this, p->defaultValue());
			break;
		}
		case iAValueType::Vector2: [[fallthrough]];
		case iAValueType::Vector3:
		{
			newWidget = new iAVectorInput(this, iAValueType::Continuous, (static_cast<int>(p->valueType()) - static_cast<int>(iAValueType::Vector2)) + 2, p->defaultValue());
			break;
		}
		case iAValueType::Vector2i: [[fallthrough]];
		case iAValueType::Vector3i:
		{
			newWidget = new iAVectorInput(this, iAValueType::Discrete,
				(static_cast<int>(p->valueType()) - static_cast<int>(iAValueType::Vector2i)) + 2, p->defaultValue());
			break;
		}
		}
		newWidget->setObjectName(p->name());	// required for ROI (parses object name)
		m_widgetList[i] = newWidget;
		containerLayout->addWidget(newWidget, i, 1);
	}

	// Try to resize dialog so that all controls are visible without scrolling; but use at most 2/3 of screen width and height
	m_container->setLayout(containerLayout);
	int maxWidth = 2 * screen()->geometry().width() / 3;
	int maxHeight = 2 * screen()->geometry().height() / 3;
	const int ScrollBarSize = 20;
	int desiredWidth = containerLayout->minimumSize().width() + ScrollBarSize;
	int desiredHeight = containerLayout->minimumSize().height() + ScrollBarSize;
	scrollArea->setMinimumWidth(std::min(desiredWidth, maxWidth));
	scrollArea->setMinimumHeight(std::min(desiredHeight, maxHeight));
	scrollArea->setWidget(m_container);
	scrollArea->setWidgetResizable(true);

	// make scrollArea widgets backround transparent
	QPalette pal = scrollArea->palette();
	pal.setColor(scrollArea->backgroundRole(), Qt::transparent);
	scrollArea->setPalette(pal);

	gridLayout->addWidget(scrollArea, 1, 0);
	gridLayout->addWidget(m_buttonBox, 2, 0);
}

void iAParameterDlg::setValue(QString const& key, QVariant const& value)
{
	auto widget = paramWidget(key);
	if (!widget)
	{
		return;
	}
	auto param = std::find_if(m_parameters.begin(), m_parameters.end(), [key](auto p) { return p->name() == key; });
	switch ((*param)->valueType())
	{
	case iAValueType::FilterParameters: [[fallthrough]];
	case iAValueType::Continuous: [[fallthrough]];
	case iAValueType::String:
		qobject_cast<QLineEdit*>(widget)->setText(value.toString());
		break;
	case iAValueType::Text:
		qobject_cast<QPlainTextEdit*>(widget)->setPlainText(value.toString());
		break;
	case iAValueType::FilterName:
		qobject_cast<QPushButton*>(widget)->setText(value.toString());
		break;
	case iAValueType::Discrete:
		qobject_cast<QSpinBox*>(widget)->setValue(value.toInt());
		break;
	case iAValueType::Boolean:
		qobject_cast<QCheckBox*>(widget)->setChecked(value.toBool());
		break;
	case iAValueType::Categorical:
		qobject_cast<QComboBox*>(widget)->setCurrentText(value.toString());
		break;
	case iAValueType::FileNameOpen: [[fallthrough]];
	case iAValueType::FileNamesOpen: [[fallthrough]];
	case iAValueType::FileNameSave: [[fallthrough]];
	case iAValueType::Folder:
		qobject_cast<iAFileChooserWidget*>(widget)->setText(value.toString());
		break;
	case iAValueType::Vector2: [[fallthrough]];
	case iAValueType::Vector3: [[fallthrough]];
	case iAValueType::Vector2i: [[fallthrough]];
	case iAValueType::Vector3i:
		qobject_cast<iAVectorInput*>(widget)->setValue(value);
		break;
	case iAValueType::Color:
		qobject_cast<iAColorInput*>(widget)->setValue(value);
		break;
	default:
		LOG(lvlError,
			QString("iAParameterDlg::setValue: value type %1 not implemented (key: %2)")
				.arg(static_cast<int>((*param)->valueType()))
				.arg(key));
	}
}

void  iAParameterDlg::setSourceMdi(iAMdiChild* child, iAMainWindow* mainWnd)
{
	assert(child);
	m_sourceMdiChild = child;
	m_mainWnd = mainWnd;
	connect(child, &iAMdiChild::closed, this, &iAParameterDlg::sourceChildClosed);
}

QWidget* iAParameterDlg::paramWidget(QString const& key)
{
	auto param = std::find_if(m_parameters.begin(), m_parameters.end(), [key](auto p) { return p->name() == key; });
	if (param == m_parameters.end())
	{
		LOG(lvlError, QString("iAParameterDlg::paramWidget: no parameter with key '%1' exists!").arg(key));
		return nullptr;
	}
	return m_widgetList[param - m_parameters.begin()];
}

void iAParameterDlg::selectFilter()
{
	QPushButton* sender = qobject_cast<QPushButton*>(QObject::sender());
	iAFilterSelectionDlg dlg(this, sender->text());
	if (dlg.exec())
	{
		QString filterName = dlg.selectedFilterName();
		auto idx = m_widgetList.indexOf(sender);
		if (idx < m_widgetList.size() - 1 && m_filterWithParameters.indexOf(idx) != -1 &&
			m_sourceMdiChild)	// TODO: if possible, get rid of sourceMdi?
		{
			auto filter = iAFilterRegistry::filter(filterName);
			int filterID = iAFilterRegistry::filterID(filterName);
			auto runner = iAFilterRunnerRegistry::filterRunner(filterID)->create();
			auto paramValues = runner->loadParameters(filter, m_sourceMdiChild);
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
	auto img = m_sourceMdiChild->firstImageData();
	if (!img)
	{
		return;
	}
	QObjectList children = m_container->children();
	for (int i = 0; i < 3; ++i)
	{
		m_roi[i] = 0;
		m_roi[i + 3] = img->GetDimensions()[i];
	}
	for (int i = 0; i < children.size(); ++i)
	{
		auto input = dynamic_cast<iAVectorInput*>(children.at(i));
		if (input && (input->objectName().contains("Index") || input->objectName().contains("Size")))
		{
			connect(input, &iAVectorInput::valueChanged, this, &iAParameterDlg::updatedROI);
			updateROIPart(input->objectName(), input->value());
		}
	}
	m_sourceMdiChild->setROIVisible(true);
	m_sourceMdiChild->updateROI(m_roi);
}

void iAParameterDlg::updatedROI(QVariant value)
{
	if (m_sourceMdiChildClosed)
	{
		return;
	}
	QString senderName = QObject::sender()->objectName();
	updateROIPart(senderName, value);
	m_sourceMdiChild->updateROI(m_roi);
}

void iAParameterDlg::updateROIPart(QString const & partName, QVariant value)
{
	auto vals = variantToVector<int>(value);
	size_t baseIdx = partName.contains("Index") ? 0 : 3;
	const int MinVal = 0;
	m_roi[baseIdx + 0] = vals[0] >= MinVal ? vals[0] : MinVal;
	m_roi[baseIdx + 1] = vals[1] >= MinVal ? vals[1] : MinVal;
	m_roi[baseIdx + 2] = vals[2] >= MinVal ? vals[2] : MinVal;
}

void iAParameterDlg::sourceChildClosed()
{
	m_sourceMdiChildClosed = true;
}

QVariantMap iAParameterDlg::parameterValues() const
{
	QVariantMap result;
	QStringList msgs;
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
				msgs << p->name();
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
			result.insert(p->name(), t->isChecked());
			break;
		}
		case iAValueType::Categorical:
		{
			QComboBox* t = qobject_cast<QComboBox*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->currentText());
			break;
		}
		default: [[fallthrough]];
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
		case iAValueType::FileNameOpen: [[fallthrough]];
		case iAValueType::FileNamesOpen: [[fallthrough]];
		case iAValueType::FileNameSave: [[fallthrough]];
		case iAValueType::Folder:
		{
			iAFileChooserWidget* t = qobject_cast<iAFileChooserWidget*>(m_widgetList[i]);
			assert(t);
			result.insert(p->name(), t->text());
			break;
		}
		case iAValueType::Vector2: [[fallthrough]];
		case iAValueType::Vector3: [[fallthrough]];
		case iAValueType::Vector2i: [[fallthrough]];
		case iAValueType::Vector3i:
		{
			iAVectorInput* t = qobject_cast<iAVectorInput*>(m_widgetList[i]);
			bool ok;
			result.insert(p->name(), t->value(&ok));
			if (!ok)
			{
				msgs << p->name();
			}
			break;
		}
		case iAValueType::Color:
		{
			iAColorInput* t = qobject_cast<iAColorInput*>(m_widgetList[i]);
			result.insert(p->name(), t->value());
			break;
		}
		}
	}
	if (!msgs.isEmpty())
	{
		QString msg = QString("Invalid values: %1. Please enter valid values!").arg(msgs.join(", "));
		LOG(lvlWarn, msg);
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
