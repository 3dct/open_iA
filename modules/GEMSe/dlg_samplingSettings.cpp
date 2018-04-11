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
#include "dlg_samplingSettings.h"

#include "iAAttributes.h"
#include "iAAttributeDescriptor.h"
#include "iAConsole.h"
#include "iAListNameMapper.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iANameMapper.h"
#include "iAParameterGeneratorImpl.h"

#include <QCheckBox>
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QShortcut>
#include <QTextStream>
#include <QStandardItemModel>

#include <cassert>

dlg_samplingSettings::dlg_samplingSettings(QWidget *parentWidget,
	QSharedPointer<iAModalityList const> modalities,
	QMap<QString, QString> const & values):
	dlg_samplingSettingsUI(parentWidget)
{
	assert(modalities->size() > 0);
	QSharedPointer<iAModality const> mod0 = modalities->Get(0);
	m_modalityCount = modalities->size();
	m_imagePixelCount = mod0->GetHeight() * mod0->GetWidth() * mod0->GetDepth();

	// assemble modality parameter input on the fly:

	QGridLayout* gridLay = dynamic_cast<QGridLayout*>(layout());
	m_startLine = gridLay->rowCount()-2;

	gridLay->addWidget(wdButtonBar, m_startLine+1, 0, 1, 4);
	
	cbSamplingMethod->clear();
	auto & paramGens = GetParameterGenerators();
	for (QSharedPointer<iAParameterGenerator> paramGen : paramGens)
	{
		cbSamplingMethod->addItem(paramGen->GetName());
	}
	cbSamplingMethod->setCurrentIndex(1);

	SetInputsFromMap(values);

	connect(leParamDescriptor, SIGNAL(editingFinished()), this, SLOT(ParameterDescriptorChanged()));
	connect(pbChooseOutputFolder, SIGNAL(clicked()), this, SLOT(ChooseOutputFolder()));
	connect(pbChooseParameterDescriptor, SIGNAL(clicked()), this, SLOT(ChooseParameterDescriptor()));
	connect(pbChooseExecutable, SIGNAL(clicked()), this, SLOT(ChooseExecutable()));
	connect(pbSaveSettings, SIGNAL(clicked()), this, SLOT(SaveSettings()));
	connect(pbLoadSettings, SIGNAL(clicked()), this, SLOT(LoadSettings()));

	connect (pbRun, SIGNAL(clicked()), this, SLOT(accept()));
	connect (pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
};


// methods for storing and loading all settings values:
// {

bool SetTextValue(QMap<QString, QString> values, QString name, QLineEdit* edit)
{
	if (values.contains(name))
	{
		edit->setText(values[name]);
		return true;
	}
	return false;
}

void SetSpinBoxValue(QMap<QString, QString> values, QString name, QSpinBox* edit)
{
	if (values.contains(name))
	{
		bool ok;
		int value = values[name].toInt(&ok);
		if (!ok)
		{
			DEBUG_LOG(QString("Invalid value '%1' for input '%2'").arg(values[name]).arg(name));
			return;
		}
		edit->setValue(value);
	}
}

void SetCheckValue(QMap<QString, QString> values, QString name, QCheckBox* checkBox)
{
	if (values.contains(name))
	{
		checkBox->setChecked(values[name] == "true");
	}
}

void SetComboBoxValue(QMap<QString, QString> values, QString name, QComboBox* comboBox)
{
	if (values.contains(name))
	{
		bool ok;
		int idx = values[name].toInt(&ok);
		if (!ok)
		{
			DEBUG_LOG(QString("Invalid value '%1' for input '%2'").arg(values[name]).arg(name));
			return;
		}
		comboBox->setCurrentIndex(idx);
	}
}

void ParameterInputs::DeleteGUI()
{
	delete label;
	DeleteGUIComponents();
}

void NumberParameterInputs::RetrieveInputValues(QMap<QString, QString> & values)
{
	QString name(label->text());
	values.insert(QString("%1 From").arg(name), from->text());
	values.insert(QString("%1 To").arg(name), to->text());
	if (logScale)
	{
		values.insert(QString("%1 Log").arg(name), logScale->isChecked() ? "true" : "false");
	}
}

void NumberParameterInputs::ChangeInputValues(QMap<QString, QString> const & values)
{
	QString name(label->text());
	SetTextValue(values, QString("%1 From").arg(name), from);
	SetTextValue(values, QString("%1 To").arg(name), to);
	if (logScale)
	{
		SetCheckValue(values, QString("%1 Log").arg(name), logScale);
	}
}

void NumberParameterInputs::DeleteGUIComponents()
{
	delete from;
	delete to;
	delete logScale;
}

void AdjustMinMax(QSharedPointer<iAAttributeDescriptor> desc, QString valueText)
{
	bool ok;
	double value = valueText.toDouble(&ok);
	if (desc->ValueType() == Categorical ||
		desc->ValueType() == Discrete)
	{
		int value = valueText.toInt(&ok);
	}
	if (!ok)
	{
		DEBUG_LOG(QString("Value '%1' for parameter %2 is not valid!").arg(valueText).arg(desc->Name()));
		return;
	}
	desc->AdjustMinMax(value);
}

QSharedPointer<iAAttributeDescriptor> NumberParameterInputs::GetCurrentDescriptor()
{
	assert(descriptor->ValueType() == Discrete || descriptor->ValueType() == Continuous);
	QString pName(label->text());
	QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
		pName,
		iAAttributeDescriptor::Parameter,
		descriptor->ValueType()));
	desc->SetNameMapper(descriptor->NameMapper());	// might not be needed, namemapper should only be necessary for categorical attributes
	AdjustMinMax(desc, from->text());
	AdjustMinMax(desc, to->text());
	if (logScale)
	{
		desc->SetLogScale(logScale->isChecked());
	}
	return desc;
}

QString CategoryParameterInputs::GetFeatureString()
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
			// descriptor->NameMapper()->GetShortName(i + descriptor->Min());
		}
	}
	return result;
}

void CategoryParameterInputs::RetrieveInputValues(QMap<QString, QString> & values)
{
	QString name(label->text());
	QString featureString = GetFeatureString();
	values.insert(name, featureString);
}

void CategoryParameterInputs::ChangeInputValues(QMap<QString, QString> const & values)
{
	QString name(label->text());
	if (!values.contains(name))
		return;
	QStringList enabledOptions = values[name].split(",");
	int curOption = 0;
	for (int i = 0; i < m_features.size() && curOption < enabledOptions.size(); ++i)
	{	// short names? descriptor->NameMapper()->GetShortName(i + descriptor->Min())
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

void CategoryParameterInputs::DeleteGUIComponents()
{
	for (int i = 0; i < m_features.size(); ++i)
	{
		delete m_features[i];
	}
}

QSharedPointer<iAAttributeDescriptor> CategoryParameterInputs::GetCurrentDescriptor()
{
	QString pName(label->text());
	assert(descriptor->ValueType() == Categorical);
	QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
		pName,
		iAAttributeDescriptor::Parameter,
		descriptor->ValueType()));
	QStringList names;
	for (int i = 0; i < m_features.size(); ++i)
	{
		if (m_features[i]->isChecked())
		{
			names.append(m_features[i]->text());
		}
	}
	QSharedPointer<iAListNameMapper> nameMapper(new iAListNameMapper(names));
	desc->SetNameMapper(nameMapper);
	desc->AdjustMinMax(0);
	desc->AdjustMinMax(names.size()-1);
	return desc;
	
}

void dlg_samplingSettings::SetInputsFromMap(QMap<QString, QString> const & values)
{
	SetTextValue(values, "Executable", leExecutable);
	SetTextValue(values, "AdditionalArguments", leAdditionalArguments);
	SetTextValue(values, "OutputFolder", leOutputFolder);
	SetTextValue(values, "PipelineName", lePipelineName);
	SetSpinBoxValue(values, "LabelCount", sbLabelCount);
	SetSpinBoxValue(values, "NumberOfSamples", sbNumberOfSamples);
	SetComboBoxValue(values, "SamplingMethod", cbSamplingMethod);
	SetCheckValue(values, "SubfolderPerSample", cbSeparateFolder);
	SetCheckValue(values, "CalculateCharacteristics", cbCalcChar);
	SetTextValue(values, "ImageBaseName", leImageBaseName);

	if (SetTextValue(values, "ParameterDescriptor", leParamDescriptor))
	{
		ParameterDescriptorChanged();
		for (int i = 0; i < m_paramInputs.size(); ++i)
		{
			m_paramInputs[i]->ChangeInputValues(values);
		}
	}
}


void dlg_samplingSettings::GetValues(QMap<QString, QString> & values) const
{
	values.clear();
	values.insert("Executable", leExecutable->text());
	values.insert("AdditionalArguments", leAdditionalArguments->text());
	values.insert("OutputFolder", leOutputFolder->text());
	values.insert("PipelineName", lePipelineName->text());
	values.insert("LabelCount", sbLabelCount->text());
	values.insert("NumberOfSamples", sbNumberOfSamples->text());
	values.insert("ParameterDescriptor", leParamDescriptor->text());
	values.insert("SamplingMethod", QString("%1").arg(cbSamplingMethod->currentIndex()));
	values.insert("SubfolderPerSample", cbSeparateFolder->isChecked()? "true" : "false");
	values.insert("CalculateCharacteristics", cbCalcChar->isChecked() ? "true" : "false");
	values.insert("ImageBaseName", leImageBaseName->text());

	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		m_paramInputs[i]->RetrieveInputValues(values);
	}
}


namespace
{
	QString const KeyValueSeparator(": ");
}


void dlg_samplingSettings::SaveSettings()
{
	QString fileName = QFileDialog::getSaveFileName(
		this,
		"Store Sampling Settings",
		QString(),
		"Sampling Settings File (*.ssf);;");
	if (fileName.isEmpty())
		return;
	QMap<QString, QString> settings;
	GetValues(settings);


	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		DEBUG_LOG(QString("Cannot open file '%1' for writing!").arg(fileName));
		return;
	}
	QTextStream stream(&file);
	for (QString key : settings.keys())
	{
		stream << key << KeyValueSeparator << settings[key] << endl;
	}
}

void dlg_samplingSettings::LoadSettings()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		"Store Sampling Settings",
		QString(),
		"Sampling Settings File (*.ssf);;");
	if (fileName.isEmpty())
		return;
	QMap<QString, QString> settings;
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
	SetInputsFromMap(settings);
}


// }

QSharedPointer<iAParameterGenerator> dlg_samplingSettings::GetGenerator()
{
	return GetParameterGenerators()[cbSamplingMethod->currentIndex()];
}


void dlg_samplingSettings::ChooseParameterDescriptor()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameter Descriptor"),
		QString(), // TODO get directory of current file
		tr("Parameter Descriptor Text File (*.txt);;All Files (*);;"));
	if (!fileName.isEmpty())
	{
		leParamDescriptor->setText(fileName);
	}
	LoadDescriptor(leParamDescriptor->text());
}


void dlg_samplingSettings::ChooseExecutable()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Executable"),
		QString(), // TODO get directory of current file
		tr("Windows Executable (*.exe);;Batch Script (*.bat);;Shell Script (*.sh);;Any Executable (*);;"));
	if (!fileName.isEmpty())
	{
		leExecutable->setText(fileName);
	}
}

void dlg_samplingSettings::ParameterDescriptorChanged()
{
	// load parameter descriptor from file
	LoadDescriptor(leParamDescriptor->text());
}

QSharedPointer<ParameterInputs> CreateParameterLine(
	QString const & pName,
	QSharedPointer<iAAttributeDescriptor> descriptor,
	QGridLayout* gridLay,
	int curGridLine)
{
	QSharedPointer<ParameterInputs> result;

	if (descriptor->ValueType() == Categorical)
	{
		auto categoryInputs = new CategoryParameterInputs();
		QWidget* w = new QWidget();
		QGridLayout* checkGridLay = new QGridLayout();
		for (int categoryIdx = descriptor->Min(); categoryIdx <= descriptor->Max(); ++categoryIdx)
		{
			QCheckBox* checkBox = new QCheckBox(descriptor->NameMapper()->GetName(categoryIdx));
			categoryInputs->m_features.push_back(checkBox);
			checkGridLay->addWidget(checkBox, (categoryIdx - descriptor->Min()) / 3, static_cast<int>(categoryIdx - descriptor->Min()) % 3);
		}
		w->setLayout(checkGridLay);
		gridLay->addWidget(w, curGridLine, 1, 1, 3);
		result = QSharedPointer<ParameterInputs>(categoryInputs);
	}
	else
	{
		auto numberInputs = new NumberParameterInputs();
		numberInputs->from = new QLineEdit(QString::number(descriptor->Min(),
			descriptor->ValueType() != Continuous? 'd' : 'g',
			descriptor->ValueType() != Continuous ? 0 : 6));
		numberInputs->to = new QLineEdit(QString::number(descriptor->Max(),
			descriptor->ValueType() != Continuous ? 'd' : 'g',
			descriptor->ValueType() != Continuous ? 0 : 6));
		gridLay->addWidget(numberInputs->from, curGridLine, 1);
		gridLay->addWidget(numberInputs->to, curGridLine, 2);
		numberInputs->logScale = new QCheckBox("Log Scale");
		numberInputs->logScale->setChecked(descriptor->IsLogScale());
		gridLay->addWidget(numberInputs->logScale, curGridLine, 3);
		result = QSharedPointer<ParameterInputs>(numberInputs);
	}
	result->label = new QLabel(pName);
	gridLay->addWidget(result->label, curGridLine, 0);
	result->descriptor = descriptor;
	return result;
}

void dlg_samplingSettings::LoadDescriptor(QString const & fileName)
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
	m_descriptor = iAAttributes::Create(in);

	// TODO: store values from previous descriptor?
	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		m_paramInputs[i]->DeleteGUI();
	}
	if (m_descriptor->size() == 0)
	{
		DEBUG_LOG("Invalid descriptor file!");
		return;
	}
	m_paramInputs.clear();
	int curGridLine = m_startLine+1;
	QGridLayout* gridLay = dynamic_cast<QGridLayout*>(layout());
	for (int i = 0; i < m_descriptor->size(); ++i)
	{
		QString pName(m_descriptor->at(i)->Name());
		if (pName.startsWith("Mod "))
		{
			for (int m = 0; m < m_modalityCount; ++m)
			{
				QSharedPointer<ParameterInputs> pInput = CreateParameterLine(QString("Mod %1 ").arg(m) +
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
			QSharedPointer<ParameterInputs> pInput = CreateParameterLine(
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


QSharedPointer<iAAttributes> dlg_samplingSettings::GetAttributes()
{
	QSharedPointer<iAAttributes> result(new iAAttributes);
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		result->Add(m_paramInputs[l]->GetCurrentDescriptor());
	}
	return result;
}


void dlg_samplingSettings::ChooseOutputFolder()
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


QString dlg_samplingSettings::GetOutputFolder() const
{
	QString outDir = leOutputFolder->text();
	return outDir.replace("\\", "/");
}


QString dlg_samplingSettings::GetAdditionalArguments() const
{
	return leAdditionalArguments->text();
}

QString dlg_samplingSettings::GetPipelineName() const
{
	return lePipelineName->text();
}

QString dlg_samplingSettings::GetExecutable() const
{
	return leExecutable->text();
}

int dlg_samplingSettings::GetSampleCount() const
{
	return sbNumberOfSamples->value();
}

int dlg_samplingSettings::GetLabelCount() const
{
	return sbLabelCount->value();
}

QString dlg_samplingSettings::GetImageBaseName() const
{
	return leImageBaseName->text();
}

bool dlg_samplingSettings::GetSeparateFolder() const
{
	return cbSeparateFolder->isChecked();
}

bool dlg_samplingSettings::GetCalcChar() const
{
	return cbCalcChar->isChecked();
}
