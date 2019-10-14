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
#pragma once

#include "ui_samplingSettings.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <QMap>

class iAAttributes;
class iAAttributeDescriptor;
class iAModalityList;
class iAParameterGenerator;

class QCheckBox;
class QShortcut;

typedef iAQTtoUIConnector<QDialog, Ui_samplingSettings> dlg_samplingSettingsUI;

class iAParameterInputs
{
public:
	virtual ~iAParameterInputs() {}
	QLabel* label;
	QSharedPointer<iAAttributeDescriptor> descriptor;
	iAParameterInputs():
		label(0)
	{}
	virtual void retrieveInputValues(QMap<QString, QString> & values) =0;
	virtual void changeInputValues(QMap<QString, QString> const & values) =0;
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
	void retrieveInputValues(QMap<QString, QString> & values) override;
	void changeInputValues(QMap<QString, QString> const & values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
private:
	void deleteGUIComponents() override;
};

class iACategoryParameterInputs : public iAParameterInputs
{
public:
	QVector<QCheckBox*> m_features;
	QString featureString();
	void retrieveInputValues(QMap<QString, QString> & values) override;
	void changeInputValues(QMap<QString, QString> const & values) override;
	QSharedPointer<iAAttributeDescriptor> currentDescriptor() override;
private:
	void deleteGUIComponents() override;
};

class dlg_samplingSettings : public dlg_samplingSettingsUI
{
	Q_OBJECT
public:
	dlg_samplingSettings(QWidget *parentWidget,
		QSharedPointer<iAModalityList const> modalities,
		QMap<QString, QString> const & values);
	QSharedPointer<iAParameterGenerator> GetGenerator();
	QSharedPointer<iAAttributes> GetAttributes();
	QString GetOutputFolder() const;
	QString GetExecutable() const;
	QString GetAdditionalArguments() const;
	QString GetPipelineName() const;
	int GetSampleCount() const;
	int labelCount() const;
	void GetValues(QMap<QString, QString> & values) const;
	QString GetImageBaseName() const;
	bool GetSeparateFolder() const;
	bool GetCalcChar() const;
private slots:
	void chooseOutputFolder();
	void chooseParameterDescriptor();
	void chooseExecutable();
	void parameterDescriptorChanged();
	void saveSettings();
	void loadSettings();
private:
	void setInputsFromMap(QMap<QString, QString> const & values);
	void loadDescriptor(QString const & fileName);

	int m_nbOfSamples;
	double m_imagePixelCount;
	int m_startLine;
	int m_modalityCount;
	QSharedPointer<iAAttributes> m_descriptor;
	QString m_descriptorFileName;
	QVector<QSharedPointer<iAParameterInputs> > m_paramInputs;
};
