/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "ui_samplingSettings.h"

#include "MetaFilters_export.h"

#include <iAAttributes.h>
#include <iASettings.h>
#include <qthelper/iAQTtoUIConnector.h>
#include <qthelper/iAWidgetSettingsMapper.h>

#include <QMap>


class iAAttributeDescriptor;
class iAModalityList;
class iASamplingMethod;

class QCheckBox;
class QShortcut;


using dlg_samplingSettingsUI = iAQTtoUIConnector<QDialog, Ui_samplingSettings>;

class iAParameterInputs
{
public:
	QLabel* label;
	QSharedPointer<iAAttributeDescriptor> descriptor;
	iAParameterInputs();
	virtual ~iAParameterInputs();
	virtual void retrieveInputValues(iASettings & values) =0;
	virtual void changeInputValues(iASettings const & values) =0;
	virtual QSharedPointer<iAAttributeDescriptor> currentDescriptor() = 0;
};

class MetaFilters_API iASamplingSettingsDlg : public dlg_samplingSettingsUI
{
	Q_OBJECT
public:
	iASamplingSettingsDlg(QWidget* parentWdgt, int inputImageCount,
		iASettings const & values);
	QSharedPointer<iAAttributes> parameterRanges();
	QSharedPointer<iAAttributes> parameterSpecs();
	void getValues(iASettings & values) const;
private slots:
	void chooseOutputFolder();
	void chooseParameterDescriptor();
	void chooseExecutable();
	void parameterDescriptorChanged();
	void saveSettings();
	void loadSettings();
	void algoTypeChanged();
	void selectFilter();
	void runClicked();
	void outputBaseChanged();
	void samplingMethodChanged();
private:
	void setInputsFromMap(iASettings const & values);
	void setParameters(QSharedPointer<iAAttributes> params);
	void setParameterValues(iASettings const& values);
	void setParametersFromFilter(QString const& filterName);
	void setParametersFromFile(QString const& fileName);

	int m_startLine;
	int m_inputImageCount;
	QString m_lastParamsFileName, m_lastFilterName;
	QVector<QSharedPointer<iAParameterInputs> > m_paramInputs;
	iAWidgetMap m_widgetMap;
	iAQRadioButtonVector m_rgAlgorithmType;
	QSharedPointer<iAAttributes> m_paramSpecs;
};
