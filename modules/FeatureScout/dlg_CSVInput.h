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
#pragma once

#include "ui_CsvInput.h"
#include "csv_config.h"
#include "DataTable.h"

#include <vtkTable.h>
#include <vtkSmartPointer.h>

#include <QMetaType>

class QSettings;

//! Loads custom csv file with data preview
//! Settings can be adapted and saved under a specified format name,
//! columns can be pre-selected to be shown in FeatureScout
class dlg_CSVInput : public QDialog, public Ui_CsvInput
{
Q_OBJECT
	typedef DataIO::DataTable dataTable;
	typedef dataTable* csvTable_ptr;
	typedef csvConfig::CTInputObjectType FiberPoreType;
public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/
	void setPath(QString const & path);
	const csvConfig::configPararams & getConfigParameters() const;
	const QVector<uint>& getEntriesSelInd();

	inline const QSharedPointer<QStringList> getHeaderSelection()
	{
		return this->m_selHeaders;
	}
	inline const ulong getTableWidth()
	{
		return this->m_confParams.tableWidth;
	}

private slots:
	void LoadCSVPreviewClicked();
	void setAllHeaders(QSharedPointer<QStringList> &allHeaders);
	void OKButtonClicked();
	//! load format based on selected input format (ex. mavi/ vg, ...)
	void LoadSelectedFormatSettings(const QString &formatName);
	//! switch between comma and column and show file preview
	void UpdateCSVPreview();
	void switchCTInputObjectType(const QString &ObjectInputType); //Switch Object Type Fiber / Pores
	//! Add format to the list of known formats
	void SaveFormatBtnClicked();
	//! Switch the inputs for the column mappings depending on what data is available
	void UpdateColumnMappingInputs();
	void UpdateLengthEditVisibility();
	void UpdateAngleEditVisibility();
private:
	//! connect signals and slots of all dialog controls
	void connectSignals();
	//! load entries with formatName or list all csv format entries. output is groups, it is empty if no features in registry
	bool CheckFeatureInRegistry(QSettings & anySetting, const QString * formatName, QStringList & groups, bool useSubGroup);
	//! store format parameters in registry
	void saveParamsToRegistry(csvConfig::configPararams & csv_params, const QString & LayoutName);
	//! load entries from registry for a given format name
	bool loadEntriesFromRegistry(QSettings & anySetting, const QString & formatName);
	//! load headers from registry
	void LoadHeaderEntriesFromReg(QStringList &HeaderEntries, const QString &HeaderNames, const QString &LayoutName);
	//! shows configuration parameters to GUI
	void showConfigParams(const csvConfig::configPararams &params);

	//! load initial settings
	void LoadFormatEntriesOnStartUp();
	void saveSettings(QSettings & anySetting, const QString & formatName, const QString & FeatureName, const QVariant & feat_value);
	void createSettingsName(QString &fullSettingsName, const QString & formatName, const QString & FeatureName, bool useSubGroup);

	void initParameters();
	void setError(const QString &ParamName, const QString & Param_value);
	void assignStartEndLine();
	void assignFormatLanguage();
	void assignInputObjectTypes();
	void assignSeparator();
	void assignSpacingUnits();
	//! assign headers and prepare map with indexes
	void assignHeaderLine();
	//! void assignColumnMappings();
	bool loadFilePreview(const int rowCount, const bool formatLoaded);
	//! checks if file exists and save it to config params
	bool checkFile(bool Layoutloaded);
	//! loading entries into table widget preview
	void loadEntries(const QString & fileName, const unsigned int nrPreviewElements, QString const & encoding);
	//! shows table with entries
	void showPreviewTable();
	void saveHeaderEntriesToReg(const QStringList & HeaderEntries, const QString & HeaderName, const QString & LayoutName);
	void clearTextControl();
	void selectAllFromTextControl();
	void setSelectedHeaderToTextControl(QStringList & sel_headers);
	//! set entries from a selected List + setting column count information for selection
	bool setSelectedEntries(const bool EnableMessageBox);
	void addSelectedHeaders(QVector<uint>& data);
	void addSingleHeaderToList(uint &currItemIdx, QString &listEntry);
	void selectSingleHeader(uint & currItemIdx, QString & listEntry);

	csvConfig::configPararams m_confParams;
	QString m_fPath;
	QString m_Error_Parameter;
	csvTable_ptr m_previewTable = nullptr;
	QString m_formatName;

	bool isFileNameValid = false;
	bool m_formatSelected = false;
	bool m_PreviewUpdated = false;
	ulong m_headersCount;

	//current headers of the table
	QSharedPointer<QStringList> m_currentHeaders;
	QSharedPointer<QStringList> m_selHeaders;
	QList<QListWidgetItem*> m_selectedHeadersList;

	QHash<QString, uint> m_hashEntries;
	QVector<uint> m_selColIdx;
};
