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
#include "iASamplingSettingsDlg.h"

#include "iAAttributes.h"
#include "iASamplingMethodImpl.h"
#include "iAParameterNames.h"

#include <iAFilterSelectionDlg.h>
#include <iAAttributeDescriptor.h>
#include <iAConsole.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iAListNameMapper.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iANameMapper.h>
#include <mainwindow.h>
#include <mdichild.h>
#include <qthelper/iAQFlowLayout.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include <cassert>


class iANumberParameterInputs : public iAParameterInputs
{
public:
	QLineEdit* from;
	QLineEdit* to;
	QCheckBox* logScale;
	iANumberParameterInputs();
	~iANumberParameterInputs();
	void retrieveInputValues(iASettings& values) override;
	void changeInputValues(iASettings const& values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
};

class iACategoryParameterInputs : public iAParameterInputs
{
public:
	QVector<QCheckBox*> m_features;
	~iACategoryParameterInputs();
	QString featureString();
	void retrieveInputValues(iASettings& values) override;
	void changeInputValues(iASettings const& values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
};

class iAOtherParameterInputs : public iAParameterInputs
{
public:
	QLineEdit* m_valueEdit;
	iAOtherParameterInputs();
	~iAOtherParameterInputs();
	void retrieveInputValues(iASettings& values) override;
	void changeInputValues(iASettings const& values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
};


iASamplingSettingsDlg::iASamplingSettingsDlg(QWidget *parentWdgt,
	int inputImageCount,
	iASettings const & values):
	dlg_samplingSettingsUI(parentWdgt),
	m_inputImageCount(inputImageCount)
{
	// to make sure that the radio button text matches the available options of the filter:
	rbBuiltIn->setText(atBuiltIn);
	rbExternal->setText(atExternal);

	m_rgAlgorithmType.push_back(rbBuiltIn);
	m_rgAlgorithmType.push_back(rbExternal);
	m_widgetMap.insert(spnAlgorithmName, lePipelineName);
	m_widgetMap.insert(spnAlgorithmType, &m_rgAlgorithmType);
	m_widgetMap.insert(spnFilter, pbFilterSelect);
	m_widgetMap.insert(spnExecutable, leExecutable);
	m_widgetMap.insert(spnParameterDescriptor, leParamDescriptor);
	m_widgetMap.insert(spnAdditionalArguments, leAdditionalArguments);
	m_widgetMap.insert(spnSamplingMethod, cbSamplingMethod);
	m_widgetMap.insert(spnNumberOfSamples, sbNumberOfSamples);
	m_widgetMap.insert(spnOutputFolder, leOutputFolder);
	m_widgetMap.insert(spnBaseName, leBaseName);
	m_widgetMap.insert(spnSubfolderPerSample, cbSeparateFolder);
	m_widgetMap.insert(spnOverwriteOutput, cbOverwriteOutput);
	m_widgetMap.insert(spnCompressOutput, cbCompressOutput);
	m_widgetMap.insert(spnContinueOnError, cbContinueOnError);
	m_widgetMap.insert(spnComputeDerivedOutput, cbCalcChar);
	m_widgetMap.insert(spnNumberOfLabels, sbLabelCount);

	m_widgetMap.insert(spnBaseSamplingMethod, cbBaseSamplingMethod);
	m_widgetMap.insert(spnSensitivityDelta, sbSensitivityDelta);

	m_startLine = parameterLayout->rowCount();

	QStringList methods(samplingMethodNames());
	cbSamplingMethod->addItems(methods);
	cbSamplingMethod->setCurrentIndex(1);

	methods.removeAll(iASamplingMethodName::GlobalSensitivity);
	cbBaseSamplingMethod->addItems(methods);
	cbBaseSamplingMethod->setCurrentIndex(1);

	QTextDocument* sampleFilterDescDoc = new QTextDocument(textSamplingDescription); // set parent so it will get deleted along with it
	sampleFilterDescDoc->setHtml(SampleFilterDescription);
	textSamplingDescription->setDocument(sampleFilterDescDoc);

	setInputsFromMap(values);

	connect(leParamDescriptor, &QLineEdit::editingFinished, this, &iASamplingSettingsDlg::parameterDescriptorChanged);
	connect(pbChooseOutputFolder, &QPushButton::clicked, this, &iASamplingSettingsDlg::chooseOutputFolder);
	connect(pbChooseParameterDescriptor, &QPushButton::clicked, this, &iASamplingSettingsDlg::chooseParameterDescriptor);
	connect(pbChooseExecutable, &QPushButton::clicked, this, &iASamplingSettingsDlg::chooseExecutable);
	connect(tbSaveSettings, &QToolButton::clicked, this, &iASamplingSettingsDlg::saveSettings);
	connect(tbLoadSettings, &QToolButton::clicked, this, &iASamplingSettingsDlg::loadSettings);
	connect(rbBuiltIn, &QRadioButton::toggled, this, &iASamplingSettingsDlg::algoTypeChanged);
	connect(rbExternal, &QRadioButton::toggled, this, &iASamplingSettingsDlg::algoTypeChanged);
	connect(pbFilterSelect, &QPushButton::clicked, this, &iASamplingSettingsDlg::selectFilter);
	connect(leOutputFolder, &QLineEdit::editingFinished, this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(leBaseName, &QLineEdit::editingFinished, this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(cbSeparateFolder, &QCheckBox::toggled, this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(sbNumberOfSamples, QOverload<int>::of(&QSpinBox::valueChanged), this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(cbSamplingMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iASamplingSettingsDlg::samplingMethodChanged);

	connect(pbRun, &QPushButton::clicked, this, &iASamplingSettingsDlg::runClicked);
	connect(pbCancel, &QPushButton::clicked, this, &iASamplingSettingsDlg::reject);
};

namespace
{
	int ContinuousPrecision = 6;
	const QString SelectFilterDefaultText("Select Filter ...");
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
		bool isCategorical = descriptor->valueType() == Categorical;
		if (isCategorical || descriptor->valueType() == Boolean)
		{
			auto categoryInputs = new iACategoryParameterInputs();
			QWidget* w = new QWidget();
			iAQFlowLayout* checkLay = new iAQFlowLayout();
			int minVal = isCategorical ? descriptor->min() : 0,
			    maxVal = isCategorical ? descriptor->max() : 1;
			checkLay->setContentsMargins(0, 0, 0, 0);
			checkLay->setSpacing(4);
			for (int categoryIdx = minVal; categoryIdx <= maxVal; ++categoryIdx)
			{
				QString title = isCategorical ? descriptor->nameMapper()->name(categoryIdx) : categoryIdx == 0 ? "false" : "true";
				QCheckBox* checkBox = new QCheckBox(title);
				categoryInputs->m_features.push_back(checkBox);
				checkLay->addWidget(checkBox);
			}
			w->setLayout(checkLay);
			gridLay->addWidget(w, curGridLine, 1, 1, 3);
			result = QSharedPointer<iAParameterInputs>(categoryInputs);
		}
		else if (descriptor->valueType() == Continuous || descriptor->valueType() == Discrete)
		{
			auto numberInputs = new iANumberParameterInputs();
			numberInputs->from = new QLineEdit(QString::number(
				descriptor->min() == std::numeric_limits<double>::lowest()? 0 : descriptor->min(),
				descriptor->valueType() != Continuous ? 'd' : 'g',
				descriptor->valueType() != Continuous ? 0 : ContinuousPrecision));
			numberInputs->to = new QLineEdit(QString::number(
				descriptor->max() == std::numeric_limits<double>::max() ? 0 : descriptor->max(),
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
			auto otherInputs = new iAOtherParameterInputs();
			otherInputs->m_valueEdit->setText(descriptor->valueType() == FileNameSave ? "" :
				descriptor->defaultValue().toString());
			otherInputs->m_valueEdit->setReadOnly(descriptor->valueType() == FileNameSave);
			gridLay->addWidget(otherInputs->m_valueEdit, curGridLine, 1, 1, 3);
			result = QSharedPointer<iAParameterInputs>(otherInputs);
			// DEBUG_LOG(QString("Don't know how to handle parameters with type %1").arg(descriptor->valueType()));
		}
		result->label = new QLabel(pName);
		gridLay->addWidget(result->label, curGridLine, 0);
		result->descriptor = descriptor;
		return result;
	}
}


iAParameterInputs::iAParameterInputs() :
	label(nullptr)
{}

iAParameterInputs::~iAParameterInputs()
{
	delete label;
}


iANumberParameterInputs::iANumberParameterInputs() :
	iAParameterInputs(),
	from(nullptr),
	to(nullptr),
	logScale(nullptr)
{}

iANumberParameterInputs::~iANumberParameterInputs()
{
	delete from;
	delete to;
	delete logScale;
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
	adjustMinMax(desc, from->text());
	adjustMinMax(desc, to->text());
	if (logScale)
	{
		desc->setLogScale(logScale->isChecked());
	}
	return desc;
}


iACategoryParameterInputs::~iACategoryParameterInputs()
{
	for (auto f : m_features)
	{
		delete f;
	}
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

QSharedPointer<iAAttributeDescriptor> iACategoryParameterInputs::currentDescriptor()
{
	QString pName(label->text());
	assert(descriptor->valueType() == Categorical || descriptor->valueType() == Boolean);
	QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
		pName,
		iAAttributeDescriptor::Parameter,
		descriptor->valueType()));
	QStringList names;
	for (auto f : m_features)
	{
		if (f->isChecked())
		{
			names.append(f->text());
		}
	}
	desc->setDefaultValue(names);
	return desc;
}


iAOtherParameterInputs::iAOtherParameterInputs() :
	m_valueEdit(new QLineEdit)
{}

iAOtherParameterInputs::~iAOtherParameterInputs()
{
	delete m_valueEdit;
}

void iAOtherParameterInputs::retrieveInputValues(iASettings& values)
{
	QString name(label->text());
	values.insert(name, m_valueEdit->text());
}

void iAOtherParameterInputs::changeInputValues(iASettings const& values)
{
	QString name(label->text());
	if (values.contains(name))
	{
		m_valueEdit->setText(values[name].toString());
	}
}

QSharedPointer<iAAttributeDescriptor> iAOtherParameterInputs::currentDescriptor()
{
	QString pName(label->text());
	QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
		pName,
		iAAttributeDescriptor::Parameter,
		descriptor->valueType()));
	desc->setDefaultValue(m_valueEdit->text());
	return desc;
}


void iASamplingSettingsDlg::setInputsFromMap(iASettings const & values)
{
	::loadSettings(values, m_widgetMap);
	algoTypeChanged();
	if (!values.contains(spnFilter) || values[spnFilter].toString().isEmpty())
	{
		pbFilterSelect->setText(SelectFilterDefaultText);
	}
	setParameterValues(values);
	outputBaseChanged();
	samplingMethodChanged();
}

void iASamplingSettingsDlg::algoTypeChanged()
{
	bool isExternal = rbExternal->isChecked();
	if (isExternal)
	{
		parameterDescriptorChanged();
	}
	else
	{
		setParametersFromFilter(pbFilterSelect->text());
	}
	pbFilterSelect->setEnabled(!isExternal);
	leExecutable->setEnabled(isExternal);
	pbChooseExecutable->setEnabled(isExternal);
	leParamDescriptor->setEnabled(isExternal);
	pbChooseParameterDescriptor->setEnabled(isExternal);
	leAdditionalArguments->setEnabled(isExternal);
}

void iASamplingSettingsDlg::getValues(iASettings& values) const
{
	values.clear();
	::saveSettings(values, m_widgetMap);
	if (values[spnFilter].toString() == SelectFilterDefaultText)
	{
		values[spnFilter] = "";
	}
	
	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		m_paramInputs[i]->retrieveInputValues(values);
	}
}

void iASamplingSettingsDlg::setParameterValues(iASettings const& values)
{
	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		m_paramInputs[i]->changeInputValues(values);
	}
}

void iASamplingSettingsDlg::saveSettings()
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

void iASamplingSettingsDlg::loadSettings()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		"Load Sampling Settings",
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

void iASamplingSettingsDlg::selectFilter()
{
	QPushButton* sender = qobject_cast<QPushButton*>(QObject::sender());
	iAFilterSelectionDlg filterSelectionDlg(this, sender->text());
	if (!filterSelectionDlg.exec())
	{
		return;
	}
	QString filterName = filterSelectionDlg.selectedFilterName();
	sender->setText(filterName);
	setParametersFromFilter(filterName);
}

void iASamplingSettingsDlg::outputBaseChanged()
{
	if (!m_paramSpecs)
	{
		return;
	}
	for (int p = 0; p < m_paramSpecs->size(); ++p)
	{
		if (m_paramSpecs->at(p)->valueType() == FileNameSave)
		{
			auto inputs = dynamic_cast<iAOtherParameterInputs*>(m_paramInputs[p].data());
			assert(inputs);
			int sampleNr = 0;
			bool createSubFolder = cbSeparateFolder->isChecked();
			int numDigits = requiredDigits(sbNumberOfSamples->value());
			auto outputFolder = getOutputFolder(leOutputFolder->text(), createSubFolder, sampleNr, numDigits);
			auto outFile = getOutputFileName(outputFolder, leBaseName->text(),
				createSubFolder, sampleNr, numDigits) + m_paramSpecs->at(p)->defaultValue().toString();
			inputs->m_valueEdit->setText(QString("Example: %1 (Set automatically during sampling)").arg(outFile));
		}
	}
}

void iASamplingSettingsDlg::samplingMethodChanged()
{
	globalSensitivitySettingsWidget->setVisible(cbSamplingMethod->currentText() == iASamplingMethodName::GlobalSensitivity);
}

void iASamplingSettingsDlg::chooseParameterDescriptor()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameter Descriptor"),
		QString(), // TODO get directory of current file
		tr("Parameter Descriptor Text File (*.txt);;All Files (*);;"));
	if (!fileName.isEmpty())
	{
		leParamDescriptor->setText(fileName);
	}
	setParametersFromFile(leParamDescriptor->text());
}

void iASamplingSettingsDlg::chooseExecutable()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Executable"),
		QString(), // TODO get directory of current file
		tr("Windows Executable (*.exe);;Batch Script (*.bat);;Shell Script (*.sh);;Any Executable (*);;"));
	if (!fileName.isEmpty())
	{
		leExecutable->setText(fileName);
	}
}

void iASamplingSettingsDlg::parameterDescriptorChanged()
{
	// load parameter descriptor from file
	setParametersFromFile(leParamDescriptor->text());
}

void iASamplingSettingsDlg::setParametersFromFile(QString const& fileName)
{
	if (fileName == m_lastParamsFileName)
	{	// nothing changed, we don't need to load again
		return;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Couldn't open parameter descriptor file '%1'\n").arg(fileName));
		return;
	}
	QTextStream in(&file);
	auto attributes = createAttributes(in);
	setParameters(attributes);
	m_lastParamsFileName = fileName;
	m_lastFilterName.clear();  // if we change to built-in, it should reload parameters
}


void iASamplingSettingsDlg::setParametersFromFilter(QString const& filterName)
{
	if (filterName == m_lastFilterName)
	{	// nothing changed, we don't need to load again
		return;
	}
	auto filter = iAFilterRegistry::filter(filterName);
	if (!filter)
	{
		DEBUG_LOG(QString("Invalid filter name '%1'").arg(filterName));
	}
	auto params = QSharedPointer<iAAttributes>(new iAAttributes(filter->parameters()));
	setParameters(params);

	QTextDocument* sampleFilterDescDoc = new QTextDocument(textAlgorithmDescription); // set parent so it will get deleted along with it
	sampleFilterDescDoc->setHtml(filter->description());
	textAlgorithmDescription->setDocument(sampleFilterDescDoc);

	m_lastFilterName = filterName;
	m_lastParamsFileName.clear();   // if we change to external, it should reload parameters
}

void iASamplingSettingsDlg::setParameters(QSharedPointer<iAAttributes> params)
{
	m_paramSpecs = params;
	m_paramInputs.clear();
	int curGridLine = m_startLine;
	for (auto p: *params.data())
	{
		QString pName(p->name());
		if (pName.startsWith("Mod "))
		{
			for (int m = 0; m < m_inputImageCount; ++m)
			{
				QSharedPointer<iAParameterInputs> pInput = createParameterLine(QString("Mod %1 ").arg(m) +
					pName.right(pName.length() - 4),
					p,
					parameterLayout,
					curGridLine);
				++curGridLine;
				m_paramInputs.push_back(pInput);
			}
		}
		else
		{
			QSharedPointer<iAParameterInputs> pInput = createParameterLine(
				pName,
				p,
				parameterLayout,
				curGridLine);
			curGridLine++;
			m_paramInputs.push_back(pInput);
		}
	}
}

QSharedPointer<iAAttributes> iASamplingSettingsDlg::parameterRanges()
{
	QSharedPointer<iAAttributes> result(new iAAttributes);
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		result->push_back(m_paramInputs[l]->currentDescriptor());
	}
	return result;
}

QSharedPointer<iAAttributes> iASamplingSettingsDlg::parameterSpecs()
{
	return m_paramSpecs;
}

void iASamplingSettingsDlg::chooseOutputFolder()
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

void iASamplingSettingsDlg::runClicked()
{
	QString minStr = QString::number(std::numeric_limits<double>::lowest(), 'g', ContinuousPrecision);
	QString maxStr = QString::number(std::numeric_limits<double>::max(), 'g', ContinuousPrecision);
	QString msg;
	if (!m_paramSpecs)
	{
		msg += QString("Parameter specifications not set, cannot run!");
	}
	if (m_paramInputs.size() != m_paramSpecs->size())
	{
		msg += QString("Number of shown parameters (=%1) is not the same "
			"as the number of parameters for the filter (=%2)!")
			.arg(m_paramInputs.size()).arg(m_paramSpecs->size());
	}
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		auto desc = m_paramInputs[l]->currentDescriptor();
		if (desc->valueType() == Continuous || desc->valueType() == Discrete)
		{
			auto curMinStr = QString::number(desc->min(), 'g'),
			     curMaxStr = QString::number(desc->max(), 'g');
			if ( !std::isfinite(desc->min()) || minStr == curMinStr)
			{
				msg += QString("Parameter '%1': invalid minimum value!").arg(desc->name());
			}
			if ( !std::isfinite(desc->max()) || maxStr == curMaxStr)
			{
				msg += QString("Parameter '%1': invalid maximum value!").arg(desc->name());
			}
			if (desc->min() > desc->max())
			{
				msg += QString("Parameter '%1': Minimum value must be smaller than or equal to maximum "
					"(current minimum %2 is bigger than maximum %3)!").arg(desc->name())
					.arg(desc->min()).arg(desc->max());
			}
			if (desc->min() < m_paramSpecs->at(l)->min() ||
				desc->max() > m_paramSpecs->at(l)->max())
			{
				msg += QString("Parameter '%1': Specified interval (%2, %3) outside of "
					"valid range for this parameter (%4, %5)").arg(desc->name())
					.arg(desc->min()).arg(desc->max())
					.arg(m_paramSpecs->at(l)->min()).arg(m_paramSpecs->at(l)->max());
			}
		}
		if (desc->valueType() == Categorical && desc->defaultValue().toString().size() == 0)
		{
			msg += QString("Parameter '%1': Currently, no value is selected; you must select at least one value!").arg(desc->name());
		}
	}
	if (rbBuiltIn->isChecked())
	{
		if (pbFilterSelect->text() == SelectFilterDefaultText)
		{
			msg += "Built-in sampling: No filter selected!";
		}
		else
		{
			QString filterName = pbFilterSelect->text();
			auto filter = iAFilterRegistry::filter(filterName);
			if (!filter)
			{
				msg += "Built-in sampling: Invalid filter name - no filter found with that name!";
			}
			else
			{
				auto mainWnd = dynamic_cast<MainWindow*>(parentWidget());
				if (mainWnd)
				{
					auto child = mainWnd->activeChild<MdiChild>();
					int curChildInputCount = child->modalities()->size();
					int childCount = mainWnd->childList<MdiChild>().size();
					int inputCount = (curChildInputCount + childCount - 1);
					if (filter->requiredInputs() > inputCount)
					{
						msg += QString("Filter requires more inputs (%1) "
							"than the number of datasets currently loaded (%2)!")
							.arg(filter->requiredInputs())
							.arg(inputCount);
					}
				}
			}
		}
	}
	else if (rbExternal->isChecked() && (leExecutable->text().isEmpty() || leParamDescriptor->text().isEmpty()))
	{
		msg += "External sampling: No executable and/or parameter descriptor chosen!";
	}
	if (!msg.isEmpty())
	{
		QMessageBox::warning(this, "Invalid configuration", msg);
		return;
	}
	accept();
}
