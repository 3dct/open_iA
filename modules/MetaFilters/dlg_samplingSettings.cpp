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
#include "dlg_samplingSettings.h"

#include "iAAttributes.h"
#include "iAParameterGeneratorImpl.h"
#include "iASampleParameterNames.h"

#include <iAAttributeDescriptor.h>
#include <iAConsole.h>
#include <iAListNameMapper.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iANameMapper.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include <cassert>

dlg_samplingSettings::dlg_samplingSettings(QWidget *parentWidget,
	int inputImageCount,
	iASettings const & values):
	dlg_samplingSettingsUI(parentWidget),
	m_inputImageCount(inputImageCount)
{
	m_widgetMap.insert(spnAlgorithmName, lePipelineName);
	//m_widgetMap.insert(spnAlgorithmType, rgAlgorithmType);
	//m_widgetMap.insert(spnFilter, pbFilterSelect);
	m_widgetMap.insert(spnExecutable, leExecutable);
	m_widgetMap.insert(spnParameterDescriptor, leParamDescriptor);
	m_widgetMap.insert(spnAdditionalArguments, leAdditionalArguments);
	m_widgetMap.insert(spnSamplingMethod, cbSamplingMethod);
	m_widgetMap.insert(spnNumberOfSamples, sbNumberOfSamples);
	m_widgetMap.insert(spnOutputFolder, leOutputFolder);
	m_widgetMap.insert(spnBaseName, leImageBaseName);
	m_widgetMap.insert(spnSubfolderPerSample, cbSeparateFolder);
	m_widgetMap.insert(spnAbortOnError, cbAbortOnError);
	m_widgetMap.insert(spnComputeDerivedOutput, cbCalcChar);
	m_widgetMap.insert(spnNumberOfLabels, sbLabelCount);

	m_startLine = parameterLayout->rowCount();

	cbSamplingMethod->clear();
	auto & paramGens = GetParameterGenerators();
	for (QSharedPointer<iAParameterGenerator> paramGen : paramGens)
	{
		cbSamplingMethod->addItem(paramGen->name());
	}
	cbSamplingMethod->setCurrentIndex(1);

	setInputsFromMap(values);

	connect(leParamDescriptor, &QLineEdit::editingFinished, this, &dlg_samplingSettings::parameterDescriptorChanged);
	connect(pbChooseOutputFolder, &QPushButton::clicked, this, &dlg_samplingSettings::chooseOutputFolder);
	connect(pbChooseParameterDescriptor, &QPushButton::clicked, this, &dlg_samplingSettings::chooseParameterDescriptor);
	connect(pbChooseExecutable, &QPushButton::clicked, this, &dlg_samplingSettings::chooseExecutable);
	connect(pbSaveSettings, &QPushButton::clicked, this, &dlg_samplingSettings::saveSettings);
	connect(pbLoadSettings, &QPushButton::clicked, this, &dlg_samplingSettings::loadSettings);

	connect (pbRun, &QPushButton::clicked, this, &dlg_samplingSettings::runClicked);
	connect (pbCancel, &QPushButton::clicked, this, &dlg_samplingSettings::reject);
};

namespace
{
	int ContinuousPrecision = 6;
	bool setTextValue(iASettings const& values, QString const& name, QLineEdit* edit)
	{
		if (values.contains(name))
		{
			auto & v = values[name];
			QString txt = (v.type() == QVariant::Double) ? QString::number(v.toDouble(), 'g', ContinuousPrecision) : v.toString();
			edit->setText(txt);
			return true;
		}
		return false;
	}

	void setCheckValue(iASettings const& values, QString const& name, QCheckBox* checkBox)
	{
		if (values.contains(name))
		{
			checkBox->setChecked(values[name] == "true");
		}
	}
	QString const KeyValueSeparator(": ");

	QSharedPointer<iAParameterInputs> createParameterLine(
		QString const& pName,
		QSharedPointer<iAAttributeDescriptor> descriptor,
		QGridLayout* gridLay,
		int curGridLine)
	{
		QSharedPointer<iAParameterInputs> result;
		// merge with common input / iAParameter dlg ?
		if (descriptor->valueType() == Categorical)
		{
			auto categoryInputs = new iACategoryParameterInputs();
			QWidget* w = new QWidget();
			QGridLayout* checkGridLay = new QGridLayout();
			for (int categoryIdx = descriptor->min(); categoryIdx <= descriptor->max(); ++categoryIdx)
			{
				QCheckBox* checkBox = new QCheckBox(descriptor->nameMapper()->name(categoryIdx));
				categoryInputs->m_features.push_back(checkBox);
				checkGridLay->addWidget(checkBox, (categoryIdx - descriptor->min()) / 3, static_cast<int>(categoryIdx - descriptor->min()) % 3);
			}
			w->setLayout(checkGridLay);
			gridLay->addWidget(w, curGridLine, 1, 1, 3);
			result = QSharedPointer<iAParameterInputs>(categoryInputs);
		}
		else if (descriptor->valueType() == Continuous || descriptor->valueType() == Discrete)
		{
			auto numberInputs = new iANumberParameterInputs();
			numberInputs->from = new QLineEdit(QString::number(descriptor->min(),
				descriptor->valueType() != Continuous ? 'd' : 'g',
				descriptor->valueType() != Continuous ? 0 : ContinuousPrecision));
			numberInputs->to = new QLineEdit(QString::number(descriptor->max(),
				descriptor->valueType() != Continuous ? 'd' : 'g',
				descriptor->valueType() != Continuous ? 0 : ContinuousPrecision));
			gridLay->addWidget(numberInputs->from, curGridLine, 1);
			gridLay->addWidget(numberInputs->to, curGridLine, 2);
			numberInputs->logScale = new QCheckBox("Log Scale");
			numberInputs->logScale->setChecked(descriptor->isLogScale());
			gridLay->addWidget(numberInputs->logScale, curGridLine, 3);
			result = QSharedPointer<iAParameterInputs>(numberInputs);
		}
		else
		{
			DEBUG_LOG(QString("Don't know how to handle parameters with type %1").arg(descriptor->valueType()));
		}
		result->label = new QLabel(pName);
		gridLay->addWidget(result->label, curGridLine, 0);
		result->descriptor = descriptor;
		return result;
	}
}

void iAParameterInputs::deleteGUI()
{
	delete label;
	deleteGUIComponents();
}


void iANumberParameterInputs::retrieveInputValues(iASettings & values)
{
	QString name(label->text());
	values.insert(QString("%1 From").arg(name), from->text());
	values.insert(QString("%1 To").arg(name), to->text());
	if (logScale)
	{
		values.insert(QString("%1 Log").arg(name), logScale->isChecked() ? "true" : "false");
	}
}

void iANumberParameterInputs::changeInputValues(iASettings const & values)
{
	QString name(label->text());
	setTextValue(values, QString("%1 From").arg(name), from);
	setTextValue(values, QString("%1 To").arg(name), to);
	if (logScale)
	{
		setCheckValue(values, QString("%1 Log").arg(name), logScale);
	}
}

void iANumberParameterInputs::deleteGUIComponents()
{
	delete from;
	delete to;
	delete logScale;
}

void adjustMinMax(QSharedPointer<iAAttributeDescriptor> desc, QString valueText)
{
	bool ok;
	double value = valueText.toDouble(&ok);
	if (desc->valueType() == Categorical ||
		desc->valueType() == Discrete)
	{
		/*int value =*/ valueText.toInt(&ok);
	}
	if (!ok)
	{
		DEBUG_LOG(QString("Value '%1' for parameter %2 is not valid!").arg(valueText).arg(desc->name()));
		return;
	}
	desc->adjustMinMax(value);
}

QSharedPointer<iAAttributeDescriptor> iANumberParameterInputs::currentDescriptor()
{
	assert(descriptor->valueType() == Discrete || descriptor->valueType() == Continuous);
	QString pName(label->text());
	QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
		pName,
		iAAttributeDescriptor::Parameter,
		descriptor->valueType()));
	desc->setNameMapper(descriptor->nameMapper());	// might not be needed, namemapper should only be necessary for categorical attributes
	adjustMinMax(desc, from->text());
	adjustMinMax(desc, to->text());
	if (logScale)
	{
		desc->setLogScale(logScale->isChecked());
	}
	return desc;
}


QString iACategoryParameterInputs::featureString()
{
	QString result;
	for (int i = 0; i < m_features.size(); ++i)
	{
		if (m_features[i]->isChecked())
		{
			if (!result.isEmpty())
			{
				result += ",";
			}
			result += m_features[i]->text();
			// distinguish between short name for storing and long name for display?
			// descriptor->nameMapper()->GetShortName(i + descriptor->Min());
		}
	}
	return result;
}

void iACategoryParameterInputs::retrieveInputValues(iASettings & values)
{
	QString name(label->text());
	values.insert(name, featureString());
}

void iACategoryParameterInputs::changeInputValues(iASettings const & values)
{
	QString name(label->text());
	if (!values.contains(name))
	{
		return;
	}
	QStringList enabledOptions = values[name].toString().split(",");
	int curOption = 0;
	for (int i = 0; i < m_features.size() && curOption < enabledOptions.size(); ++i)
	{	// short names? descriptor->nameMapper()->GetShortName(i + descriptor->Min())
		if (m_features[i]->text() == enabledOptions[curOption])
		{
			m_features[i]->setChecked(true);
			curOption++;
		}
	}
	if (curOption != enabledOptions.size())
	{
		DEBUG_LOG(QString("Inconsistent state: not all stored, enabled options found for parameter '%1'").arg(name));
	}
}

void iACategoryParameterInputs::deleteGUIComponents()
{
	for (int i = 0; i < m_features.size(); ++i)
	{
		delete m_features[i];
	}
}

QSharedPointer<iAAttributeDescriptor> iACategoryParameterInputs::currentDescriptor()
{
	QString pName(label->text());
	assert(descriptor->valueType() == Categorical);
	QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
		pName,
		iAAttributeDescriptor::Parameter,
		descriptor->valueType()));
	QStringList names;
	for (int i = 0; i < m_features.size(); ++i)
	{
		if (m_features[i]->isChecked())
		{
			names.append(m_features[i]->text());
		}
	}
	QSharedPointer<iAListNameMapper> nameMapper(new iAListNameMapper(names));
	desc->setNameMapper(nameMapper);
	desc->adjustMinMax(0);
	desc->adjustMinMax(names.size()-1);
	return desc;

}


void dlg_samplingSettings::setInputsFromMap(iASettings const & values)
{
	::loadSettings(values, m_widgetMap);
	
	if (values.contains("Parameter descriptor"))
	{
		parameterDescriptorChanged();
		for (int i = 0; i < m_paramInputs.size(); ++i)
		{
			m_paramInputs[i]->changeInputValues(values);
		}
	}
}

void dlg_samplingSettings::getValues(iASettings & values) const
{
	values.clear();
	::saveSettings(values, m_widgetMap);

	// only external at the moment, make sure it is set:
	values["Algorithm type"] = "External";
	
	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		m_paramInputs[i]->retrieveInputValues(values);
	}
}

void dlg_samplingSettings::saveSettings()
{
	QString fileName = QFileDialog::getSaveFileName(
		this,
		"Store Sampling Settings",
		QString(),
		"Sampling Settings File (*.ssf);;");
	if (fileName.isEmpty())
	{
		return;
	}
	// TODO: common iASettings storage framework?
	iASettings settings;
	getValues(settings);

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		DEBUG_LOG(QString("Cannot open file '%1' for writing!").arg(fileName));
		return;
	}
	QTextStream stream(&file);
	for (QString key : settings.keys())
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		stream << key << KeyValueSeparator << settings[key].toString() << Qt::endl;
#else
		stream << key << KeyValueSeparator << settings[key].toString() << endl;
#endif
	}
}

void dlg_samplingSettings::loadSettings()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		"Store Sampling Settings",
		QString(),
		"Sampling Settings File (*.ssf);;");
	if (fileName.isEmpty())
	{
		return;
	}
	iASettings settings;
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		DEBUG_LOG(QString("Cannot open file '%1' for reading!").arg(fileName));
		return;
	}
	QTextStream in(&file);
	while (!in.atEnd())
	{
		QString line = in.readLine();
		int sepPos = line.indexOf(KeyValueSeparator);
		if (sepPos == -1)
		{
			DEBUG_LOG(QString("Invalid line '%1'").arg(line));
		}
		QString key = line.left(sepPos);
		QString value = line.right(line.length() - (sepPos + KeyValueSeparator.length()));
		settings.insert(key, value);
	}
	setInputsFromMap(settings);
}

void dlg_samplingSettings::chooseParameterDescriptor()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameter Descriptor"),
		QString(), // TODO get directory of current file
		tr("Parameter Descriptor Text File (*.txt);;All Files (*);;"));
	if (!fileName.isEmpty())
	{
		leParamDescriptor->setText(fileName);
	}
	loadDescriptor(leParamDescriptor->text());
}

void dlg_samplingSettings::chooseExecutable()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Executable"),
		QString(), // TODO get directory of current file
		tr("Windows Executable (*.exe);;Batch Script (*.bat);;Shell Script (*.sh);;Any Executable (*);;"));
	if (!fileName.isEmpty())
	{
		leExecutable->setText(fileName);
	}
}

void dlg_samplingSettings::parameterDescriptorChanged()
{
	// load parameter descriptor from file
	loadDescriptor(leParamDescriptor->text());
}

void dlg_samplingSettings::loadDescriptor(QString const & fileName)
{
	if (fileName == m_descriptorFileName)
	{
		// same filename as before, we don't need to load again
		return;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Couldn't open parameter descriptor file '%1'\n").arg(fileName));
		return;
	}
	QTextStream in(&file);
	m_descriptor = createAttributes(in);

	// TODO: store values from previous descriptor?
	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		m_paramInputs[i]->deleteGUI();
	}
	if (m_descriptor->size() == 0)
	{
		DEBUG_LOG("Invalid descriptor file!");
		return;
	}
	m_paramInputs.clear();
	int curGridLine = m_startLine+1;
	QGridLayout* gridLay = parameterLayout;
	for (int i = 0; i < m_descriptor->size(); ++i)
	{
		QString pName(m_descriptor->at(i)->name());
		if (pName.startsWith("Mod "))
		{
			for (int m = 0; m < m_inputImageCount; ++m)
			{
				QSharedPointer<iAParameterInputs> pInput = createParameterLine(QString("Mod %1 ").arg(m) +
					pName.right(pName.length() - 4),
					m_descriptor->at(i),
					gridLay,
					curGridLine);
				curGridLine++;
				m_paramInputs.push_back(pInput);
			}
		}
		else
		{
			QSharedPointer<iAParameterInputs> pInput = createParameterLine(
				pName,
				m_descriptor->at(i),
				gridLay,
				curGridLine);
			curGridLine++;
			m_paramInputs.push_back(pInput);
		}
	}
	gridLay->addWidget(wdButtonBar, curGridLine, 0, 1, 4);
	m_descriptorFileName = fileName;
}

QSharedPointer<iAAttributes> dlg_samplingSettings::parameterRanges()
{
	QSharedPointer<iAAttributes> result(new iAAttributes);
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		result->push_back(m_paramInputs[l]->currentDescriptor());
	}
	return result;
}

void dlg_samplingSettings::chooseOutputFolder()
{
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly);
	QString outFolder = dialog.getExistingDirectory(this, "Choose Output Folder");
	if (outFolder != "")
	{
		leOutputFolder->setText(outFolder);
	}
}

void dlg_samplingSettings::runClicked()
{
	QString minStr = QString::number(std::numeric_limits<double>::lowest(), 'g', ContinuousPrecision);
	QString maxStr = QString::number(std::numeric_limits<double>::max(), 'g', ContinuousPrecision);
	QString msg;
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		auto desc = m_paramInputs[l]->currentDescriptor();
		if (desc->valueType() == Continuous || desc->valueType() == Discrete)
		{
			auto curMinStr = QString::number(desc->min(), 'g'),
			     curMaxStr = QString::number(desc->max(), 'g');
			if ( !std::isfinite(desc->min()) || minStr == curMinStr)
			{
				msg += QString("Parameter '%1': invalid minimum value!").arg(desc->name()).arg(desc->min());
			}
			if ( !std::isfinite(desc->max()) || maxStr == curMaxStr)
			{
				msg += QString("Parameter '%1': invalid maximum value!").arg(desc->name()).arg(desc->max());
			}
		}
	}
	if (!msg.isEmpty())
	{
		QMessageBox::warning(this, "Invalid input", msg);
		return;
	}
	accept();
}
