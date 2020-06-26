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
#pragma once

#include "ui_samplingSettings.h"

#include "MetaFilters_export.h"

#include <iASettings.h>
#include <qthelper/iAQTtoUIConnector.h>
#include <qthelper/iAWidgetSettingsMapper.h>

#include <QMap>

class iAAttributes;
class iAAttributeDescriptor;
class iAModalityList;
class iAParameterGenerator;

class QCheckBox;
class QShortcut;

using dlg_samplingSettingsUI = iAQTtoUIConnector<QDialog, Ui_samplingSettings>;

class iAParameterInputs
{
public:
	virtual ~iAParameterInputs() {}
	QLabel* label;
	QSharedPointer<iAAttributeDescriptor> descriptor;
	iAParameterInputs():
		label(0)
	{}
	virtual void retrieveInputValues(iASettings & values) =0;
	virtual void changeInputValues(iASettings const & values) =0;
	void deleteGUI();
	virtual QSharedPointer<iAAttributeDescriptor> currentDescriptor() = 0;
private:
	virtual void deleteGUIComponents() = 0;
};

class iANumberParameterInputs: public iAParameterInputs
{
public:
	QLineEdit* from;
	QLineEdit* to;
	QCheckBox* logScale;
	iANumberParameterInputs():
		iAParameterInputs(),
		from(0),
		to(0),
		logScale(0)
	{}
	void retrieveInputValues(iASettings& values) override;
	void changeInputValues(iASettings const & values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
private:
	void deleteGUIComponents() override;
};

class iACategoryParameterInputs : public iAParameterInputs
{
public:
	QVector<QCheckBox*> m_features;
	QString featureString();
	void retrieveInputValues(iASettings& values) override;
	void changeInputValues(iASettings const & values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
private:
	void deleteGUIComponents() override;
};

class MetaFilters_API dlg_samplingSettings : public dlg_samplingSettingsUI
{
	Q_OBJECT
public:
	dlg_samplingSettings(QWidget *parentWidget, int inputImageCount,
		iASettings const & values);
	QSharedPointer<iAParameterGenerator> generator();
	QSharedPointer<iAAttributes> parameterRanges();
	QString outputFolder() const;
	QString executable() const;
	QString additionalArguments() const;
	QString algorithmName() const;
	int sampleCount() const;
	int labelCount() const;
	void getValues(iASettings & values) const;
	QString outBaseName() const;
	bool useSeparateFolder() const;
	bool computeDerivedOutput() const;
private slots:
	void chooseOutputFolder();
	void chooseParameterDescriptor();
	void chooseExecutable();
	void parameterDescriptorChanged();
	void saveSettings();
	void loadSettings();
private:
	void setInputsFromMap(iASettings const & values);
	void loadDescriptor(QString const & fileName);

	int m_startLine;
	int m_inputImageCount;
	QSharedPointer<iAAttributes> m_descriptor;
	QString m_descriptorFileName;
	QVector<QSharedPointer<iAParameterInputs> > m_paramInputs;
	iAWidgetMap m_widgetMap;
};
