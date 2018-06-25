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
#include "io/csv_config.h"
#include "io/DataTable.h"

#include <QMetaType>

#include "vtkTable.h"
#include <vtkSmartPointer.h>

class QSettings;

namespace FeatureScoutCSV
{
	struct csvRegKeys
	{
		csvRegKeys()
		{
			initParam();
		}
		void initParam()
		{
			str_settingsName = "FeatureScoutCSV";
			str_formatName = "FormatName";
			str_headerName = "HeaderEntries";
			str_allHeaders = "AllHeaders";
			str_fileName = "FileName";
			str_reg_useEndline = "useEndLine";
			str_reg_startLine = "StartLine";
			str_reg_EndLine = "Endline";
			str_reg_colSeparator = "ColumnSeperator";
			str_reg_languageFormat = "LanguageFormat";
			str_reg_Spacing = "Spacing";
			str_reg_Units = "Microns";
			str_reg_FiberPoreData = "InputObjectType";
		}
		//Values to be written in registry
		QVariant v_startLine;
		QVariant v_useEndline;
		QVariant v_endLine;
		QVariant v_colSeparator;
		QVariant v_languageFormat;
		QVariant v_Spacing;
		QVariant v_Units;
		QVariant v_FiberPoreObject; //for fibers or pores
		QVariant v_fileName;
		QVariant v_allHeaders;

		//Names for registry
		QString str_settingsName;
		QString str_headerName;
		QString str_allHeaders;
		QString str_fileName;
		QString str_formatName;
		QString str_reg_useEndline;
		QString str_reg_startLine;
		QString str_reg_EndLine;
		QString str_reg_colSeparator;
		QString str_reg_languageFormat;
		QString str_reg_Spacing;
		QString str_reg_Units;

		//for fibers or pores
		QString str_reg_FiberPoreData;
	};
}


//! Loads custom csv File with DataPreview
//! Settings can be adapted and saved for specified DataFormat
//! Enables column pre selection to be shown in FeatureScout
class dlg_CSVInput : public QDialog, public Ui_CsvInput
{
	Q_OBJECT
		typedef DataIO::DataTable dataTable;
		typedef dataTable* csvTable_ptr;
		typedef FeatureScoutCSV::csvRegKeys csv_reg;
		typedef QSharedPointer<csv_reg> cvsRegSettings_ShrdPtr;  //QSharedPointer csv_reg

		typedef csvConfig::csvSeparator csvColSeparator;
		typedef csvConfig::inputLang csvLang;
		typedef csvConfig::csv_FileFormat csvFormat;
		typedef csvConfig::CTInputObjectType FiberPoreType;
public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/

	//! load Headers from registry
	void LoadHeaderEntriesFromReg(QStringList &HeaderEntries, const QString &HeaderNames, const QString &LayoutName);
	const csvConfig::configPararams & getConfigParameters() const;
	//! shows configuration parameters to GUI
	void showConfigParams(const csvConfig::configPararams &params, const bool paramsLoaded);
	inline void setFilePath(const QString& FPath)
	{
		if(!FPath.isEmpty())
		{
			this->m_fPath = FPath;
		}
	}
	void addSelectedHeaders(QVector<uint>& data);
	void addSingleHeaderToList(uint &currItemIdx, QString &listEntry);
	const QVector<uint>& getEntriesSelInd();
	void selectSingleHeader(uint & currItemIdx, QString & listEntry);
	void setSelectedHeaderToTextControl(QStringList & sel_headers);
	//! set entries from a selected List + setting column count information for selection
	bool setSelectedEntries(const bool EnableMessageBox);

	inline const QSharedPointer<QStringList> getHeaderSelection()
	{
		return this->m_selHeaders;
	}

	inline const QSharedPointer<QStringList> getAllHeaders()
	{
		return this->m_currentHeaders;
	}

	inline const ulong getTableWidth()
	{
		return this->m_confParams->tableWidth;
	}

private slots:
	void LoadCSVPreviewClicked();
	void setAllHeaders(QSharedPointer<QStringList> &allHeaders);
	void OKButtonClicked();
	//custom file format
	void EnableCustomFormat();
	void showFormatComponents();
	//! load format based on selected input format (ex. mavi/ vg, ...)
	void LoadSelectedFormatSettings(const QString &LayoutName);
	//! switch between comma and column and show file preview
	void UpdateCSVPreview();
	void switchCTInputObjectType(const QString &ObjectInputType); //Switch Object Type Fiber / Pores
	//! clear all entries in Table
	void resetTable();
	//! Add format to the list of known formats
	void SaveFormatBtnClicked();
private:
	void connectSignals();
	//! load entries with layoutName or list all entries under FeaturescoutCSV. output is groups, it is empty if no features in registry
	bool CheckFeatureInRegistry(QSettings & anySetting, const QString * LayoutName, QStringList & groups, bool useSubGroup);

	void saveParamsToRegistry(csvConfig::configPararams & csv_params, const QString & LayoutName);
	//! load entries from registry for a configuration setting
	bool loadEntriesFromRegistry(QSettings & anySetting, const QString & LayoutName);

	//! load initial settings
	void LoadFormatEntriesOnStartUp();
	void saveSettings(QSettings & anySetting, const QString & LayoutName, const QString & FeatureName, const QVariant & feat_value);
	void createSettingsName(QString &fullSettingsName, const QString & LayoutName, const QString & FeatureName, bool useSubGroup);

	//! saving headers from registry entry
	void initParameters();
	void initBasicFormatParameters(csvLang Language, csvColSeparator FileSeparator, csvFormat FileFormat);
	void initStartEndline(unsigned long startLine, unsigned long EndLine, const bool useEndline);

	void resetDefault();
	void assignStartEndLine();
	void setError(const QString &ParamName, const QString & Param_value);
	void assignFormatLanguage();
	void assignInputObjectTypes();
	void assignSeparator();
	void assignSpacingUnits();
	//! assign headers and prepare map with indexes
	void assignHeaderLine();
	bool loadFilePreview(const int rowCount, const bool formatLoaded);
	//! checks if file exists and save it to config params
	bool checkFile(bool Layoutloaded);
	//! loading entries into table widget preview
	bool loadEntries(const QString & fileName, const unsigned int nrPreviewElements);
	//! shows table with entries
	void showPreviewTable();
	void readHeaderLine(const uint headerStartRow);

	void hideCoordinateInputs();
	void disableFormatComponents();

	void saveHeaderEntriesToReg(const QStringList & HeaderEntries, const QString & HeaderName, const QString & LayoutName);
	void clearTextControl();
	void selectAllFromTextControl();

private:

	bool useCustomformat;

	QSharedPointer<csvConfig::configPararams> m_confParams;
	QString m_fPath;
	QString m_Error_Parameter;
	csvTable_ptr m_entriesPreviewTable = nullptr;
	cvsRegSettings_ShrdPtr m_regEntries;
	QString m_LayoutName;

	bool isFileNameValid = false;
	bool isFilledWithData = false;
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
