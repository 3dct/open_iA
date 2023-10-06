// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

#include <iAAttributes.h>
#include <qthelper/iAWidgetSettingsMapper.h>

#include <QDialog>
#include <QMap>

class iAAttributeDescriptor;
class iASamplingMethod;
class Ui_samplingSettings;

class QCheckBox;
class QLabel;
class QShortcut;

class iAParameterInputs
{
public:
	QLabel* label;
	std::shared_ptr<iAAttributeDescriptor> descriptor;
	iAParameterInputs();
	virtual ~iAParameterInputs();
	virtual void retrieveInputValues(QVariantMap& values) =0;
	virtual void changeInputValues(QVariantMap const & values) =0;
	virtual std::shared_ptr<iAAttributeDescriptor> currentDescriptor() = 0;
};

class MetaFilters_API iASamplingSettingsDlg : public QDialog
{
	Q_OBJECT
public:
	iASamplingSettingsDlg(QWidget* parentWdgt, int inputImageCount,
		QVariantMap const & values);
	std::shared_ptr<iAAttributes> parameterRanges();
	std::shared_ptr<iAAttributes> parameterSpecs();
	void getValues(QVariantMap& values) const;
	std::vector<int> numOfSamplesPerParameter() const;
public slots:
	void updateNumSamples();
private slots:
	void chooseOutputFolder();
	void chooseParameterDescriptor();
	void chooseExecutable();
	void chooseParameterSetFile();
	void parameterDescriptorChanged();
	void saveSettings();
	void loadSettings();
	void algoTypeChanged();
	void selectFilter();
	void runClicked();
	void outputBaseChanged();
	void numberOfSamplesChanged();
	void samplingMethodChanged();
	void showAlgorithmInfo();
	void showSamplingInfo();
private:
	void setInputsFromMap(QVariantMap const & values);
	void setParameters(std::shared_ptr<iAAttributes> params);
	void setParameterValues(QVariantMap const& values);
	void setParametersFromFilter(QString const& filterName);
	void setParametersFromFile(QString const& fileName);
	void updateActualNumSamples();

	int m_startLine;
	int m_inputImageCount;
	QString m_lastParamsFileName, m_lastFilterName;
	QVector<std::shared_ptr<iAParameterInputs> > m_paramInputs;
	iAWidgetMap m_widgetMap;
	iAQRadioButtonVector m_rgAlgorithmType;
	std::shared_ptr<iAAttributes> m_paramSpecs;
	std::shared_ptr<Ui_samplingSettings> m_ui;
};
