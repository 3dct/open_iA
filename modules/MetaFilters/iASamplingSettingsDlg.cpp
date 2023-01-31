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
#include "iASamplingSettingsDlg.h"

#include "iAAttributes.h"
#include "iASamplingMethodImpl.h"
#include "iAParameterNames.h"
#include "ui_samplingSettings.h"

#include <iAFilterSelectionDlg.h>
#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iANameMapper.h>
#include <iAStringHelper.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <iAQFlowLayout.h>

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
	QSpinBox* numSamples;
	iANumberParameterInputs();
	~iANumberParameterInputs();
	void retrieveInputValues(QVariantMap& values) override;
	void changeInputValues(QVariantMap const& values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
};

class iACategoryParameterInputs : public iAParameterInputs
{
public:
	QVector<QCheckBox*> m_features;
	~iACategoryParameterInputs();
	QString featureString();
	void retrieveInputValues(QVariantMap& values) override;
	void changeInputValues(QVariantMap const& values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
};

class iAOtherParameterInputs : public iAParameterInputs
{
public:
	QLineEdit* m_valueEdit;
	iAOtherParameterInputs();
	~iAOtherParameterInputs();
	void retrieveInputValues(QVariantMap& values) override;
	void changeInputValues(QVariantMap const& values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
};


iASamplingSettingsDlg::iASamplingSettingsDlg(QWidget *parentWdgt,
	int inputImageCount,
	QVariantMap const & values):
	QDialog(parentWdgt),
	m_inputImageCount(inputImageCount),
	m_ui(new Ui_samplingSettings())
{
	m_ui->setupUi(this);
	// to make sure that the radio button text matches the available options of the filter:
	m_ui->rbBuiltIn->setText(atBuiltIn);
	m_ui->rbExternal->setText(atExternal);

	m_rgAlgorithmType.push_back(m_ui->rbBuiltIn);
	m_rgAlgorithmType.push_back(m_ui->rbExternal);
	m_widgetMap.insert(spnAlgorithmName, m_ui->lePipelineName);
	m_widgetMap.insert(spnAlgorithmType, &m_rgAlgorithmType);
	m_widgetMap.insert(spnFilter, m_ui->pbFilterSelect);
	m_widgetMap.insert(spnExecutable, m_ui->leExecutable);
	m_widgetMap.insert(spnParameterDescriptor, m_ui->leParamDescriptor);
	m_widgetMap.insert(spnAdditionalArguments, m_ui->leAdditionalArguments);
	m_widgetMap.insert(spnSamplingMethod, m_ui->cbSamplingMethod);
	m_widgetMap.insert(spnNumberOfSamples, m_ui->sbNumberOfSamples);
	m_widgetMap.insert(spnOutputFolder, m_ui->leOutputFolder);
	m_widgetMap.insert(spnBaseName, m_ui->leBaseName);
	m_widgetMap.insert(spnSubfolderPerSample, m_ui->cbSeparateFolder);
	m_widgetMap.insert(spnOverwriteOutput, m_ui->cbOverwriteOutput);
	m_widgetMap.insert(spnCompressOutput, m_ui->cbCompressOutput);
	m_widgetMap.insert(spnContinueOnError, m_ui->cbContinueOnError);
	m_widgetMap.insert(spnComputeDerivedOutput, m_ui->cbCalcChar);
	m_widgetMap.insert(spnNumberOfLabels, m_ui->sbLabelCount);

	m_widgetMap.insert(spnBaseSamplingMethod, m_ui->cbBaseSamplingMethod);
	m_widgetMap.insert(spnStarDelta, m_ui->sbStarDelta);
	m_widgetMap.insert(spnStarStepNumber, m_ui->sbStarStepNumber);

	m_widgetMap.insert(spnParameterSetFile, m_ui->leParameterSetFile);

	m_startLine = m_ui->parameterLayout->rowCount()-1;

	QStringList methods(samplingMethodNames());
	m_ui->cbSamplingMethod->addItems(methods);
	m_ui->cbSamplingMethod->setCurrentIndex(1);

	methods.removeAll(iASamplingMethodName::GlobalSensitivity);
	m_ui->cbBaseSamplingMethod->addItems(methods);
	m_ui->cbBaseSamplingMethod->setCurrentIndex(1);

	QTextDocument* sampleFilterDescDoc =
		new QTextDocument(m_ui->textSamplingDescription);  // set parent so it will get deleted along with it
	sampleFilterDescDoc->setHtml(SampleFilterDescription);
	m_ui->textSamplingDescription->setDocument(sampleFilterDescDoc);

	setInputsFromMap(values);

	connect(m_ui->leParamDescriptor, &QLineEdit::editingFinished, this, &iASamplingSettingsDlg::parameterDescriptorChanged);
	connect(m_ui->tbChooseOutputFolder, &QToolButton::clicked, this, &iASamplingSettingsDlg::chooseOutputFolder);
	connect(m_ui->tbChooseParameterDescriptor, &QToolButton::clicked, this, &iASamplingSettingsDlg::chooseParameterDescriptor);
	connect(m_ui->tbChooseExecutable, &QToolButton::clicked, this, &iASamplingSettingsDlg::chooseExecutable);
	connect(m_ui->tbSaveSettings, &QToolButton::clicked, this, &iASamplingSettingsDlg::saveSettings);
	connect(m_ui->tbLoadSettings, &QToolButton::clicked, this, &iASamplingSettingsDlg::loadSettings);
	connect(m_ui->rbBuiltIn, &QRadioButton::toggled, this, &iASamplingSettingsDlg::algoTypeChanged);
	connect(m_ui->rbExternal, &QRadioButton::toggled, this, &iASamplingSettingsDlg::algoTypeChanged);
	connect(m_ui->pbFilterSelect, &QPushButton::clicked, this, &iASamplingSettingsDlg::selectFilter);
	connect(m_ui->leOutputFolder, &QLineEdit::editingFinished, this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(m_ui->leBaseName, &QLineEdit::editingFinished, this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(m_ui->cbSeparateFolder, &QCheckBox::toggled, this, &iASamplingSettingsDlg::outputBaseChanged);
	connect(m_ui->sbNumberOfSamples, QOverload<int>::of(&QSpinBox::valueChanged), this, &iASamplingSettingsDlg::numberOfSamplesChanged);
	connect(m_ui->cbSamplingMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iASamplingSettingsDlg::samplingMethodChanged);
	connect(m_ui->tbAlgorithmInfo, &QToolButton::toggled, this, &iASamplingSettingsDlg::showAlgorithmInfo);
	connect(m_ui->tbSamplingInfo, &QToolButton::toggled, this, &iASamplingSettingsDlg::showSamplingInfo);
	connect(m_ui->tbChooseParameterSetFile, &QToolButton::clicked, this, &iASamplingSettingsDlg::chooseParameterSetFile);
	// initial state
	showAlgorithmInfo();
	showSamplingInfo();

	connect(m_ui->pbRun, &QPushButton::clicked, this, &iASamplingSettingsDlg::runClicked);
	connect(m_ui->pbCancel, &QPushButton::clicked, this, &iASamplingSettingsDlg::reject);
};

namespace
{
	int ContinuousPrecision = 6;
	const QString SelectFilterDefaultText("Select Filter ...");
	bool setTextValue(QVariantMap const& values, QString const& name, QLineEdit* edit)
	{
		if (values.contains(name))
		{
			auto v = values[name];
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			QString txt = (v.type() == QVariant::Double) ? QString::number(v.toDouble(), 'g', ContinuousPrecision) : v.toString();
#else
			QString txt = (v.metaType().id() == QMetaType::Double) ? QString::number(v.toDouble(), 'g', ContinuousPrecision) : v.toString();
#endif
			edit->setText(txt);
			return true;
		}
		return false;
	}

	void setCheckValue(QVariantMap const& values, QString const& name, QCheckBox* checkBox)
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
		int curGridLine, 
		iASamplingSettingsDlg* eventHandler)
	{
		QSharedPointer<iAParameterInputs> result;
		// merge with common input / iAParameter dlg ?
		bool isCategorical = descriptor->valueType() == iAValueType::Categorical;
		if (isCategorical || descriptor->valueType() == iAValueType::Boolean)
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
				QObject::connect(checkBox, &QCheckBox::toggled, eventHandler, &iASamplingSettingsDlg::updateNumSamples);
			}
			w->setLayout(checkLay);
			gridLay->addWidget(w, curGridLine, 1, 1, 3);
			result = QSharedPointer<iAParameterInputs>(categoryInputs);
		}
		else if (descriptor->valueType() == iAValueType::Continuous || descriptor->valueType() == iAValueType::Discrete)
		{
			auto numberInputs = new iANumberParameterInputs();
			numberInputs->from = new QLineEdit(QString::number(
				descriptor->min() == std::numeric_limits<double>::lowest()? 0 : descriptor->min(),
				descriptor->valueType() != iAValueType::Continuous ? 'd' : 'g',
				descriptor->valueType() != iAValueType::Continuous ? 0 : ContinuousPrecision));
			numberInputs->to = new QLineEdit(QString::number(
				descriptor->max() == std::numeric_limits<double>::max() ? 0 : descriptor->max(),
				descriptor->valueType() != iAValueType::Continuous ? 'd' : 'g',
				descriptor->valueType() != iAValueType::Continuous ? 0 : ContinuousPrecision));
			numberInputs->numSamples = new QSpinBox();
			numberInputs->numSamples->setMinimum(1);
			numberInputs->numSamples->setMaximum(9999);
			numberInputs->numSamples->setVisible(false);
			gridLay->addWidget(numberInputs->from, curGridLine, 1);
			gridLay->addWidget(numberInputs->to, curGridLine, 2);
			numberInputs->logScale = new QCheckBox("Log Scale");
			numberInputs->logScale->setChecked(descriptor->isLogScale());
			gridLay->addWidget(numberInputs->logScale, curGridLine, 3);
			gridLay->addWidget(numberInputs->numSamples, curGridLine, 4);
			QObject::connect(numberInputs->numSamples, QOverload<int>::of(&QSpinBox::valueChanged), eventHandler, &iASamplingSettingsDlg::updateNumSamples);
			result = QSharedPointer<iAParameterInputs>(numberInputs);
		}
		else
		{
			auto otherInputs = new iAOtherParameterInputs();
			otherInputs->m_valueEdit->setText(descriptor->valueType() == iAValueType::FileNameSave ? "" :
				descriptor->defaultValue().toString());
			otherInputs->m_valueEdit->setReadOnly(descriptor->valueType() == iAValueType::FileNameSave);
			gridLay->addWidget(otherInputs->m_valueEdit, curGridLine, 1, 1, 3);
			result = QSharedPointer<iAParameterInputs>(otherInputs);
			// LOG(lvlWarn, QString("Don't know how to handle parameters with type %1").arg(descriptor->valueType()));
		}
		result->label = new QLabel(pName);
		result->label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
	logScale(nullptr),
	numSamples(nullptr)
{}

iANumberParameterInputs::~iANumberParameterInputs()
{
	delete from;
	delete to;
	delete logScale;
	delete numSamples;
}

void iANumberParameterInputs::retrieveInputValues(QVariantMap& values)
{
	QString name(label->text());
	values.insert(QString("%1 From").arg(name), from->text());
	values.insert(QString("%1 To").arg(name), to->text());
	if (logScale)
	{
		values.insert(QString("%1 Log").arg(name), logScale->isChecked() ? "true" : "false");
	}
}

void iANumberParameterInputs::changeInputValues(QVariantMap const & values)
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
	if (desc->valueType() == iAValueType::Categorical ||
		desc->valueType() == iAValueType::Discrete)
	{
		/*int value =*/ valueText.toInt(&ok);
	}
	if (!ok)
	{
		LOG(lvlError, QString("Value '%1' for parameter %2 is not valid!").arg(valueText).arg(desc->name()));
		return;
	}
	desc->adjustMinMax(value);
}

QSharedPointer<iAAttributeDescriptor> iANumberParameterInputs::currentDescriptor()
{
	assert(descriptor->valueType() == iAValueType::Discrete || descriptor->valueType() == iAValueType::Continuous);
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

void iACategoryParameterInputs::retrieveInputValues(QVariantMap& values)
{
	QString name(label->text());
	values.insert(name, featureString());
}

void iACategoryParameterInputs::changeInputValues(QVariantMap const & values)
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
		LOG(lvlError, QString("Inconsistent state: not all stored, enabled options found for parameter '%1'").arg(name));
	}
}

QSharedPointer<iAAttributeDescriptor> iACategoryParameterInputs::currentDescriptor()
{
	QString pName(label->text());
	assert(descriptor->valueType() == iAValueType::Categorical || descriptor->valueType() == iAValueType::Boolean);
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

void iAOtherParameterInputs::retrieveInputValues(QVariantMap& values)
{
	QString name(label->text());
	values.insert(name, m_valueEdit->text());
}

void iAOtherParameterInputs::changeInputValues(QVariantMap const& values)
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


void iASamplingSettingsDlg::setInputsFromMap(QVariantMap const & values)
{
	::loadSettings(values, m_widgetMap);
	algoTypeChanged();
	if (!values.contains(spnFilter) || values[spnFilter].toString().isEmpty())
	{
		m_ui->pbFilterSelect->setText(SelectFilterDefaultText);
	}
	setParameterValues(values);
	outputBaseChanged();
	samplingMethodChanged();
}

void iASamplingSettingsDlg::algoTypeChanged()
{
	bool isExternal = m_ui->rbExternal->isChecked();
	if (isExternal)
	{
		parameterDescriptorChanged();
	}
	else
	{
		setParametersFromFilter(m_ui->pbFilterSelect->text());
	}
	m_ui->tbAlgorithmInfo->setEnabled(!isExternal);
	m_ui->pbFilterSelect->setEnabled(!isExternal);
	m_ui->leExecutable->setEnabled(isExternal);
	m_ui->tbChooseExecutable->setEnabled(isExternal);
	m_ui->leParamDescriptor->setEnabled(isExternal);
	m_ui->tbChooseParameterDescriptor->setEnabled(isExternal);
	m_ui->leAdditionalArguments->setEnabled(isExternal);
	m_ui->lbExternalExecutable->setEnabled(isExternal);
	m_ui->lbExternalParameterDescriptor->setEnabled(isExternal);
	m_ui->lbExternalAdditionalArguments->setEnabled(isExternal);
}

void iASamplingSettingsDlg::getValues(QVariantMap& values) const
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

std::vector<int> iASamplingSettingsDlg::numOfSamplesPerParameter() const
{
	std::vector<int> result(m_paramInputs.size(), 1);
	QVariantMap s;
	getValues(s);
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		auto desc = m_paramInputs[l]->currentDescriptor();
		if (desc->valueType() == iAValueType::Categorical || desc->valueType() == iAValueType::Boolean)
		{
			result[l] = s[desc->name()].toString().split(",").size();
		}
		else if (desc->valueType() == iAValueType::Continuous || desc->valueType() == iAValueType::Discrete)
		{
			result[l]  = dynamic_cast<iANumberParameterInputs*>(m_paramInputs[l].data())->numSamples->value();
		}
	}
	return result;
}

void iASamplingSettingsDlg::updateNumSamples()
{
	QVariantMap s;
	getValues(s);
	auto samplingMethod = createSamplingMethod(s);
	if (!samplingMethod->supportsSamplesPerParameter())
	{
		return;
	}
	auto samplesPerParam = numOfSamplesPerParameter();
	int numSamples = std::accumulate(samplesPerParam.begin(), samplesPerParam.end(), 1, std::multiplies<int>{});
	m_ui->lbNumberOfSamplesActual->setText(QString("actual: %1").arg(numSamples));
}

void iASamplingSettingsDlg::setParameterValues(QVariantMap const& values)
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
		"Sampling settings file (*.ssf);;All files (*)");
	if (fileName.isEmpty())
	{
		return;
	}
	// TODO: common settings storage framework?
	QVariantMap settings;
	getValues(settings);

	// use QSettings?
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		LOG(lvlError, QString("Cannot open file '%1' for writing!").arg(fileName));
		return;
	}
	QTextStream stream(&file);
	for (QString key : settings.keys())
	{
		stream << key << KeyValueSeparator << settings[key].toString() << Qt::endl;
	}
}

void iASamplingSettingsDlg::loadSettings()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		"Load Sampling Settings",
		QString(),
		"Sampling settings file (*.ssf);;All files (*)");
	if (fileName.isEmpty())
	{
		return;
	}
	QVariantMap settings;
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError, QString("Cannot open file '%1' for reading!").arg(fileName));
		return;
	}
	QTextStream in(&file);
	while (!in.atEnd())
	{
		QString line = in.readLine();
		int sepPos = line.indexOf(KeyValueSeparator);
		if (sepPos == -1)
		{
			LOG(lvlError, QString("Invalid line '%1'").arg(line));
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
		if (m_paramSpecs->at(p)->valueType() == iAValueType::FileNameSave)
		{
			auto inputs = dynamic_cast<iAOtherParameterInputs*>(m_paramInputs[p].data());
			assert(inputs);
			int sampleNr = 0;
			bool createSubFolder = m_ui->cbSeparateFolder->isChecked();
			int numDigits = requiredDigits(m_ui->sbNumberOfSamples->value());
			auto outputFolder = getOutputFolder(m_ui->leOutputFolder->text(), createSubFolder, sampleNr, numDigits);
			auto outFile = getOutputFileName(outputFolder, m_ui->leBaseName->text(),
				createSubFolder, sampleNr, numDigits) + m_paramSpecs->at(p)->defaultValue().toString();
			inputs->m_valueEdit->setText(QString("Example: %1 (Set automatically during sampling)").arg(outFile));
		}
	}
}
void iASamplingSettingsDlg::updateActualNumSamples()
{
	QVariantMap s;
	getValues(s);
	auto samplingMethod = createSamplingMethod(s);
	if (samplingMethod->supportsSamplesPerParameter())
	{
		samplingMethod->setSampleCount(m_ui->sbNumberOfSamples->value(), m_paramSpecs);
		m_ui->lbNumberOfSamplesActual->setText(QString("actual: %1").arg(samplingMethod->sampleCount()));
	}
}

void iASamplingSettingsDlg::numberOfSamplesChanged()
{
	updateActualNumSamples();
	outputBaseChanged();
}

void iASamplingSettingsDlg::samplingMethodChanged()
{
	QString samplingMethodName = m_ui->cbSamplingMethod->currentText();
	m_ui->gbSamplingMethodDetails->setVisible(samplingMethodName == iASamplingMethodName::GlobalSensitivity ||
		samplingMethodName == iASamplingMethodName::GlobalSensitivitySmall ||
		samplingMethodName == iASamplingMethodName::RerunSampling);
	m_ui->lbNumberOfSamples->setVisible(samplingMethodName != iASamplingMethodName::RerunSampling);
	m_ui->sbNumberOfSamples->setVisible(samplingMethodName != iASamplingMethodName::RerunSampling);
	m_ui->widgetSensitivitySamplingParameters->setVisible(samplingMethodName == iASamplingMethodName::GlobalSensitivity ||
		samplingMethodName == iASamplingMethodName::GlobalSensitivitySmall);
	m_ui->lbStarStepNumber->setVisible(samplingMethodName == iASamplingMethodName::GlobalSensitivitySmall);
	m_ui->sbStarStepNumber->setVisible(samplingMethodName == iASamplingMethodName::GlobalSensitivitySmall);
	m_ui->widgetRerunSamplingParameters->setVisible(samplingMethodName == iASamplingMethodName::RerunSampling);

	QVariantMap s;
	getValues(s);
	auto samplingMethod = createSamplingMethod(s);
	for (int p = 0; p < m_paramSpecs->size(); ++p)
	{
		auto t = m_paramSpecs->at(p)->valueType();
		if (t == iAValueType::Discrete || t == iAValueType::Continuous)
		{
			auto inputs = dynamic_cast<iANumberParameterInputs*>(m_paramInputs[p].data());
			inputs->numSamples->setVisible(samplingMethod->supportsSamplesPerParameter());
		}
	}
	m_ui->lbNumSamplesPerParam->setVisible(samplingMethod->supportsSamplesPerParameter());
	m_ui->lbNumberOfSamplesActual->setVisible(samplingMethod->supportsSamplesPerParameter());
	updateActualNumSamples();
}

void iASamplingSettingsDlg::showAlgorithmInfo()
{
	m_ui->textAlgorithmDescription->setVisible(m_ui->tbAlgorithmInfo->isChecked());
}

void iASamplingSettingsDlg::showSamplingInfo()
{
	m_ui->textSamplingDescription->setVisible(m_ui->tbSamplingInfo->isChecked());
}

void iASamplingSettingsDlg::chooseParameterDescriptor()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameter Descriptor"),
		QString(), // TODO get directory of current file
		tr("Parameter descriptor file (*.txt);;All files (*)"));
	if (!fileName.isEmpty())
	{
		m_ui->leParamDescriptor->setText(fileName);
	}
	setParametersFromFile(m_ui->leParamDescriptor->text());
}

void iASamplingSettingsDlg::chooseExecutable()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Executable"),
		QString(), // TODO get directory of current file
		tr("Windows executable (*.exe);;Batch script (*.bat);;Shell script (*.sh);;Any executable (*)"));
	if (!fileName.isEmpty())
	{
		m_ui->leExecutable->setText(fileName);
	}
}

void iASamplingSettingsDlg::parameterDescriptorChanged()
{
	// load parameter descriptor from file
	setParametersFromFile(m_ui->leParamDescriptor->text());
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
		LOG(lvlError, QString("Couldn't open parameter descriptor file '%1'\n").arg(fileName));
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
		LOG(lvlError, QString("Invalid filter name '%1'").arg(filterName));
	}
	auto params = QSharedPointer<iAAttributes>(new iAAttributes(filter->parameters()));
	setParameters(params);

	QTextDocument* sampleFilterDescDoc = new QTextDocument(m_ui->textAlgorithmDescription); // set parent so it will get deleted along with it
	sampleFilterDescDoc->setHtml(filter->description());
	m_ui->textAlgorithmDescription->setDocument(sampleFilterDescDoc);

	m_lastFilterName = filterName;
	m_lastParamsFileName.clear();   // if we change to external, it should reload parameters
}

void iASamplingSettingsDlg::setParameters(QSharedPointer<iAAttributes> params)
{
	m_paramSpecs = params;
	m_paramInputs.clear();
	int curGridLine = m_startLine;
	m_ui->parameterLayout->removeItem(m_ui->vspaceParamBottom);
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
					m_ui->parameterLayout,
					curGridLine, this);
				++curGridLine;
				m_paramInputs.push_back(pInput);
			}
		}
		else
		{
			QSharedPointer<iAParameterInputs> pInput = createParameterLine(
				pName,
				p,
				m_ui->parameterLayout,
				curGridLine, this);
			++curGridLine;
			m_paramInputs.push_back(pInput);
		}
	}
	m_ui->parameterLayout->addItem(m_ui->vspaceParamBottom, curGridLine, 1);
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

void iASamplingSettingsDlg::chooseParameterSetFile()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		"Load Parameter Set",
		QString(),
		"Parameter set file (*.csv);;All files (*)");
	if (fileName != "")
	{
		m_ui->leParameterSetFile->setText(fileName);
	}
}

void iASamplingSettingsDlg::chooseOutputFolder()
{
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly);
	QString outFolder = dialog.getExistingDirectory(this, "Choose Output Folder");
	if (outFolder != "")
	{
		m_ui->leOutputFolder->setText(outFolder);
	}
}

void iASamplingSettingsDlg::runClicked()
{
	QString minStr = QString::number(std::numeric_limits<double>::lowest(), 'g', ContinuousPrecision);
	QString maxStr = QString::number(std::numeric_limits<double>::max(), 'g', ContinuousPrecision);
	QString msg;
	if (!m_paramSpecs)
	{
		msg += QString("Parameter specifications not set, cannot run!\n");
	}
	if (m_paramInputs.size() != m_paramSpecs->size())
	{
		msg += QString("Number of shown parameters (=%1) is not the same "
			"as the number of parameters for the filter (=%2)!\n")
			.arg(m_paramInputs.size()).arg(m_paramSpecs->size());
	}
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		auto desc = m_paramInputs[l]->currentDescriptor();
		if (desc->valueType() == iAValueType::Continuous || desc->valueType() == iAValueType::Discrete)
		{
			auto curMinStr = QString::number(desc->min(), 'g'),
			     curMaxStr = QString::number(desc->max(), 'g');
			if ( !std::isfinite(desc->min()) || minStr == curMinStr)
			{
				msg += QString("Parameter '%1': invalid minimum value!\n").arg(desc->name());
			}
			if ( !std::isfinite(desc->max()) || maxStr == curMaxStr)
			{
				msg += QString("Parameter '%1': invalid maximum value!\n").arg(desc->name());
			}
			if (desc->min() > desc->max())
			{
				msg += QString("Parameter '%1': Minimum value must be smaller than or equal to maximum "
					"(current minimum %2 is bigger than maximum %3)!\n").arg(desc->name())
					.arg(desc->min()).arg(desc->max());
			}
			if (desc->min() < m_paramSpecs->at(l)->min() ||
				desc->max() > m_paramSpecs->at(l)->max())
			{
				msg += QString("Parameter '%1': Specified interval (%2, %3) outside of "
					"valid range for this parameter (%4, %5)\n").arg(desc->name())
					.arg(desc->min()).arg(desc->max())
					.arg(m_paramSpecs->at(l)->min()).arg(m_paramSpecs->at(l)->max());
			}
		}
		if ((desc->valueType() == iAValueType::Categorical || desc->valueType() == iAValueType::Boolean) &&
			desc->defaultValue().toString().size() == 0)
		{
			msg += QString("Parameter '%1': Currently, no value is selected; you must select at least one value!\n").arg(desc->name());
		}
	}
	if (m_ui->rbBuiltIn->isChecked())
	{
		if (m_ui->pbFilterSelect->text() == SelectFilterDefaultText)
		{
			msg += "Built-in sampling: No filter selected!\n";
		}
		else
		{
			QString filterName = m_ui->pbFilterSelect->text();
			auto filter = iAFilterRegistry::filter(filterName);
			if (!filter)
			{
				msg += "Built-in sampling: Invalid filter name - no filter found with that name!\n";
			}
			else
			{
				auto mainWnd = dynamic_cast<iAMainWindow*>(parentWidget());
				if (mainWnd)
				{
					auto child = mainWnd->activeMdiChild();
					if (static_cast<int>(filter->requiredImages() + filter->requiredMeshes()) > child->dataSetMap().size())
					{
						msg += QString("Filter requires more inputs (%1 image(s) + %2 mesh(es)) "
							"than the number of datasets loaded in current child (%3)!\n")
							.arg(filter->requiredImages())
							.arg(filter->requiredMeshes())
							.arg(child->dataSetMap().size());
					}
				}
			}
		}
	}
	else if (m_ui->rbExternal->isChecked() && (m_ui->leExecutable->text().isEmpty() || m_ui->leParamDescriptor->text().isEmpty()))
	{
		msg += "External sampling: No executable and/or parameter descriptor chosen!\n";
	}
	else if (m_ui->rbExternal->isChecked() && !QFileInfo(m_ui->leExecutable->text()).exists())
	{
		msg += QString("External sampling: Executable '%1' doesn't exist!\n").arg(m_ui->leExecutable->text());
	}
	if (!msg.isEmpty())
	{
		QMessageBox::warning(this, "Invalid configuration", msg);
		return;
	}
	accept();
}
