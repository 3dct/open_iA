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
#include "iACsvConfig.h"

class iACsvIO;

class QSettings;

//! Loads custom csv file with data preview
//! Settings can be adapted and saved under a specified format name
class dlg_CSVInput : public QDialog, public Ui_CsvInput
{
Q_OBJECT
public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/
	void setPath(QString const & path);
	const iACsvConfig & getConfig() const;
private slots:
	//! updates preview (e.g. when Update Preview button called)
	void selectFileBtnClicked();
	//! handles a click on the OK button
	void okBtnClicked();
	//! load format based on selected input format (ex. mavi/ vg, ...)
	void loadSelectedFormatSettings(const QString &formatName);
	//! show file preview (triggered on any update to the format settings and when Update Preview button is clicked)
	void updatePreview();
	//! switch Object Type Fiber / Pores
	void switchObjectType(const QString &inputType);
	//! Add format to the list of known formats
	void saveFormatBtnClicked();
	//! Delete format from the list of known formats
	void deleteFormatBtnClicked();
	//! Apply default format selection
	void applyFormatColumnSelection();
	//! switch the inputs for the column mappings depending on what data is available
	void updateColumnMappingInputs();
	//! called when selected columns change
	void selectedColsChanged();
	//! called when Compute Length checkbox check state changed
	void computeLengthChanged();
	//! called when Compute Angles checkbox check state changed
	void computeAngleChanged();
private:
	//! switch length mapping choice enabled based on whether to checkbox to automatically compute it is checked or not
	void updateLengthEditEnabled();
	//! switch angle mapping choices enabled based on whether to checkbox to automatically compute it is checked or not
	void updateAngleEditEnabled();
	//! initialize GUI elements
	void initParameters();
	//! connect signals and slots of all dialog controls
	void connectSignals();
	//! List all csv format entries, returned list is empty if no format definitions in registry
	QStringList getFormatListFromRegistry() const;
	//! Retrieve name of format loaded last time the dialog was open
	QString getDefaultFormat() const;
	//! Save name of format to be loaded next time dialog is opened
	void saveDefaultFormat(QString const & formatName);
	//! Save a specific format with its settings in registry
	void saveFormatToRegistry(const QString & formatName);
	//! Load entries from registry for a given format name
	bool loadFormatFromRegistry(const QString & formatName);
	//! Deletes a format from the registry
	void deleteFormatFromReg(QString const & formatName);
	//! Load selected headers from registry
	QStringList loadHeadersFromReg(const QString &formatName, const QString& entryName);
	//! Save selected headers to registry
	void saveHeadersToReg(const QString &formatName, const QString& entryName, QStringList const & headers);
	//! Shows configuration parameters in GUI
	void showConfigParams();
	//! Show selected columns from parameters in GUI
	void showSelectedCols();
	//! @{ assign methods take data from GUI and assign it to config object
	//! Assign all format settings from the GUI to the internal configuration object
	//! (except for header lines)
	void assignFormatSettings();
	//! Assign the input object type from the GUI to the internal configuration object
	void assignObjectTypes();
	//! Set entries from a selected List + setting column count information for selection
	void assignSelectedCols();
	//! @}
	//! Show column header in list widget, and prepare map with indexes
	void showColumnHeaders();
	//! void assignColumnMappings();
	//! Load file into preview table widget
	bool loadFilePreview();
	//! Checks if file exists and save it to config params
	bool checkFile();
	//! Clears the preview table (so that it doesn't contain any rows or columns
	void clearPreviewTable();

	iACsvConfig m_confParams;
	QString m_path;
	QString m_formatName;
	QVector<QComboBox*> m_mappingBoxes;
	bool m_columnMappingChoiceSet; //!< whether we have provided proper choices in the column mapping comboboxes already
};
