/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include <iAQTtoUIConnector.h>

#include <QMap>

class iAAttributes;
class iAAttributeDescriptor;
class iAModalityList;
class iAParameterGenerator;

class QCheckBox;
class QShortcut;

struct ParameterInputs
{
	QLabel* label;
	QLineEdit* from;
	QLineEdit* to;
	QCheckBox* logScale;
	QSharedPointer<iAAttributeDescriptor> descriptor;
	ParameterInputs():
		label(0),
		from(0),
		to(0),
		logScale(0)
	{}
};

typedef iAQTtoUIConnector<QDialog, Ui_samplingSettings> dlg_samplingSettingsUI;

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
	int GetLabelCount() const;
	void GetValues(QMap<QString, QString> & values) const;
private slots:
	void ChooseOutputFolder();
	void ChooseParameterDescriptor();
	void ChooseExecutable();
	void ParameterDescriptorChanged();
private:
	void SetInputsFromMap(QMap<QString, QString> const & values);
	int m_nbOfSamples;
	double m_imagePixelCount;
	int m_startLine;
	int m_modalityCount;
	QSharedPointer<iAAttributes> m_descriptor;
	QString m_descriptorFileName;
	QVector<ParameterInputs> m_paramInputs;

	void LoadDescriptor(QString const & fileName);
};
